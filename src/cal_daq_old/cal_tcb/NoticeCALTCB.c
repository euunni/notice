#include "NoticeCALTCB.h"

enum EManipInterface {kInterfaceClaim, kInterfaceRelease};

struct dev_open {
   libusb_device_handle *devh;
   uint16_t vendor_id;
   uint16_t product_id;
   int serial_id;
   struct dev_open *next;
} *ldev_open = 0;

// internal functions *********************************************************************************
static int is_device_open(libusb_device_handle *devh);
static unsigned char get_serial_id(libusb_device_handle *devh);
static void add_device(struct dev_open **list, libusb_device_handle *tobeadded,
                       uint16_t vendor_id, uint16_t product_id, int sid);
static int handle_interface_id(struct dev_open **list, uint16_t vendor_id, uint16_t product_id,
                               int sid, int interface, enum EManipInterface manip_type);
static void remove_device_id(struct dev_open **list, uint16_t vendor_id, uint16_t product_id, int sid);
libusb_device_handle* nkusb_get_device_handle(uint16_t vendor_id, uint16_t product_id, int sid);
int TCBWrite(int sid, uint32_t mid, uint32_t addr, uint32_t data);
int TCBRead(int sid, uint32_t mid, uint32_t count, uint32_t addr, unsigned char *data);
unsigned int TCBReadReg(int sid, uint32_t mid, uint32_t addr);

void CALTCBstart_DRS(int sid);
void CALTCBstop_DRS(int sid);
unsigned long CALTCBread_DRS_PLL_LOCKED(int sid, unsigned long mid);
unsigned long CALTCBread_DRAMTEST(int sid, unsigned long mid, unsigned long ch);
void CALTCBwrite_DRS_OFS(int sid, unsigned long mid, unsigned long rofs, unsigned long oofs);
void CALTCBwrite_ADC_SETUP(int sid, unsigned long mid, unsigned long addr, unsigned long data);
void CALTCBsetup_DRAM(int sid, unsigned long mid);
void CALTCBwrite_DRAMTEST(int sid, unsigned long mid, int data);
int CALTCBread_DRAM_ALIGN(int sid, unsigned long mid, int ch);
void CALTCBreset_REF_CLK(int sid, unsigned long mid);
void CALTCBwrite_DRAMDLY(int sid, unsigned long mid, int ch, int data);
void CALTCBwrite_DRAM_BITSLIP(int sid, unsigned long mid, int ch);
void CALTCBalign_DRAM(int sid, unsigned long mid);

static int is_device_open(libusb_device_handle *devh)
{
// See if the device handle "devh" is on the open device list

  struct dev_open *curr = ldev_open;
  libusb_device *dev, *dev_curr;
  int bus, bus_curr, addr, addr_curr;

  while (curr) {
    dev_curr = libusb_get_device(curr->devh);
    bus_curr = libusb_get_bus_number(dev_curr);
    addr_curr = libusb_get_device_address(dev_curr);

    dev = libusb_get_device(devh);
    bus = libusb_get_bus_number(dev);
    addr = libusb_get_device_address(dev);

    if (bus == bus_curr && addr == addr_curr) return 1;
    curr = curr->next;
  }

  return 0;
}

static unsigned char get_serial_id(libusb_device_handle *devh)
{
// Get serial id of device handle devh. devh may or may not be on open device list 'ldev_open'.
// Returns 0 if error.
  int ret;
  if (!devh) {
    return 0;
  }
  unsigned char data[1];
  ret = libusb_control_transfer(devh, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, 0xD2, 0, 0, data, 1, 1000);

  if (ret < 0) {
    fprintf(stdout, "Warning: get_serial_id: Could not get serial id.\n");
    return 0;
  }

  return data[0];
}

static void add_device(struct dev_open **list, libusb_device_handle *tobeadded,
                       uint16_t vendor_id, uint16_t product_id, int sid)
{
// Add device to the open device list

  struct dev_open *curr;

  curr = (struct dev_open *)malloc(sizeof(struct dev_open));
  curr->devh = tobeadded;
  curr->vendor_id = vendor_id;
  curr->product_id = product_id;
  curr->serial_id = sid;
  curr->next  = *list;
  *list = curr;
}

static int handle_interface_id(struct dev_open **list, uint16_t vendor_id, uint16_t product_id,
                               int sid, int interface, enum EManipInterface manip_type)
{
// Manipulate interface on the device with specified vendor id, product id and serial id
// from open device list. Claim interface if manip_type == kInterfaceClaim, release interface
// if manip_type == kInterfaceRelease. If sid == NK_SID_ANY, all devices with given vendor id
// and product id are handled.

  int ret = 0;
  if (!*list) {
    ret = -1;
    return ret;
  }

  struct dev_open *curr = *list;
  struct libusb_device_descriptor desc;
  libusb_device *dev;

  while (curr) {
    dev =libusb_get_device(curr->devh);
    if (libusb_get_device_descriptor(dev, &desc) < 0) {
      // Ignore with message
      fprintf(stdout, "Warning: remove_device: could not get device device descriptior."
                          " Ignoring.\n");
      continue;
    }
    if (desc.idVendor == vendor_id && desc.idProduct == product_id
    && (sid == 0xFF || sid == get_serial_id(curr->devh))) { 
      // Match.
      if (manip_type == kInterfaceClaim) {
        if ((ret = libusb_claim_interface(curr->devh, interface)) < 0) {
          fprintf(stdout, "Warning: handle_interface_id: Could not claim interface (%d) on device (%u, %u, %u)\n",
                  interface, vendor_id, product_id, 
sid);
        }
      }
      else if (manip_type == kInterfaceRelease) {
        if ((ret =libusb_release_interface(curr->devh, interface)) < 0) {
          fprintf(stdout, "Warning: handle_interface_id: Could not release interface (%d) on device (%u, %u, %u)\n",
                  interface, vendor_id, product_id, sid);
        }
      }
      else {
        fprintf(stderr, "Error: handle_interface_id: Unknown interface handle request: %d\n.",
                manip_type);
              
        ret = -1;
        return ret;
      }
    }
    // Move forward
    curr = curr->next;
  }

  return ret;
}

static void remove_device_id(struct dev_open **list, uint16_t vendor_id, uint16_t product_id, int sid)
{
// Close and remove device with specified vendor id, product id and serial id
// from open device list. If sid == NK_SID_ANY, all devices with given vendor id
// and product id are removed.

  if (!*list) return;

  struct dev_open *curr = *list;
  struct dev_open *prev = 0;
  struct libusb_device_descriptor desc;
  libusb_device *dev;

  while (curr) {
    dev =libusb_get_device(curr->devh);
    if (libusb_get_device_descriptor(dev, &desc) < 0) {
      // Ignore with message
      fprintf(stdout, "Warning, remove_device: could not get device device descriptior." " Ignoring.\n");
      continue;
    }
    if (desc.idVendor == vendor_id && desc.idProduct == product_id
    && (sid == 0xFF || sid == get_serial_id(curr->devh))) { 
      // Match.
      if (*list == curr) { 
        // Update head, remove current element and move cursor forward.
        *list = curr->next;
        libusb_close(curr->devh);
        free(curr); 
        curr = *list;
      }
      else {
        // Update link, remove current element and move cursor forward.
        prev->next = curr->next;
        libusb_close(curr->devh);
        free(curr); 
        curr = prev->next;
      }
    }
    else {
      // No match. Move cursor forward.
      prev = curr;
      curr = curr->next;
    }    
  }
}

libusb_device_handle* nkusb_get_device_handle(uint16_t vendor_id, uint16_t product_id, int sid) 
{
// Get open device handle with given vendor id, product id and serial id.
// Return first found device handle if sid == NK_SID_ANY.

  struct dev_open *curr = ldev_open;
  while (curr) {
    if (curr->vendor_id == vendor_id && curr->product_id == product_id) {
      if (sid == 0xFF)
        return curr->devh;
      else if (curr->serial_id == sid)
        return curr->devh;
    }

    curr = curr->next;
  }

  return 0;
}

int TCBWrite(int sid, uint32_t mid, uint32_t addr, uint32_t data)
{
  int transferred = 0;  
  const unsigned int timeout = 1000;
  //int length = 8;
  int length = 12;
  unsigned char *buffer;
  int stat = 0;
  
  if (!(buffer = (unsigned char *)malloc(length))) {
    fprintf(stderr, "TCBWrite: Could not allocate memory (size = %d\n)", length);
    return -1;
  }
  
  buffer[0] = data & 0xFF;
  buffer[1] = (data >> 8)  & 0xFF;
  buffer[2] = (data >> 16)  & 0xFF;
  buffer[3] = (data >> 24)  & 0xFF;
  
  buffer[4] = addr & 0xFF;
  buffer[5] = (addr >> 8)  & 0xFF;
  buffer[6] = (addr >> 16)  & 0xFF;
  buffer[7] = (addr >> 24)  & 0x7F;

  buffer[8] = mid & 0xFF;
  buffer[9] = (mid >> 8) & 0xFF;
  buffer[10] = (mid >> 16) & 0xFF;
  buffer[11] = (mid >> 24) & 0xFF;
  
  libusb_device_handle *devh = nkusb_get_device_handle(CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid);
  if (!devh) {
    fprintf(stderr, "TCBWrite: Could not get device handle for the device.\n");
    return -1;
  }
  
  if ((stat = libusb_bulk_transfer(devh, USB3_SF_WRITE, buffer, length, &transferred, timeout)) < 0) {
    fprintf(stderr, "TCBWrite: Could not make write request; error = %d\n", stat);
    free(buffer);
    return stat;
  }
  
  free(buffer);

  usleep(1000);

  return stat;
}

int TCBRead(int sid, uint32_t mid, uint32_t count, uint32_t addr, unsigned char *data)
{
  const unsigned int timeout = 1000; // Wait forever
  //int length = 8;
  int length = 12;
  int transferred;
  unsigned char *buffer;
  int stat = 0;
  int nbulk;
  int remains;
  int loop;
  int size = 16384; // 16 kB

  nbulk = count / 4096;
  remains = count % 4096;

  if (!(buffer = (unsigned char *)malloc(size))) {
    fprintf(stderr, "TCBRead: Could not allocate memory (size = %d\n)", size);
    return -1;
  }
  
  buffer[0] = count & 0xFF;
  buffer[1] = (count >> 8)  & 0xFF;
  buffer[2] = (count >> 16)  & 0xFF;
  buffer[3] = (count >> 24)  & 0xFF;
  
  buffer[4] = addr & 0xFF;
  buffer[5] = (addr >> 8)  & 0xFF;
  buffer[6] = (addr >> 16)  & 0xFF;
  buffer[7] = (addr >> 24)  & 0x7F;
  buffer[7] = buffer[7] | 0x80;

  buffer[8] = mid & 0xFF;
  buffer[9] = (mid >> 8) & 0xFF;
  buffer[10] = (mid >> 16) & 0xFF;
  buffer[11] = (mid >> 24) & 0xFF;

  libusb_device_handle *devh = nkusb_get_device_handle(CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid);
  if (!devh) {
    fprintf(stderr, "TCBRead: Could not get device handle for the device.\n");
    return -1;
  }

  if ((stat = libusb_bulk_transfer(devh, USB3_SF_WRITE, buffer, length, &transferred, timeout)) < 0) {
    fprintf(stderr, "TCBRead: Could not make write request; error = %d\n", stat);
    free(buffer);
    return stat;
  }

  for (loop = 0; loop < nbulk; loop++) {
    if ((stat = libusb_bulk_transfer(devh, USB3_SF_READ, buffer, size, &transferred, timeout)) < 0) {
      fprintf(stderr, "TCBRead: Could not make read request; error = %d\n", stat);
      return 1;
    }
    memcpy(data + loop * size, buffer, size);
  }

  if (remains) {
    if ((stat = libusb_bulk_transfer(devh, USB3_SF_READ, buffer, remains * 4, &transferred, timeout)) < 0) {
      fprintf(stderr, "TCBRead: Could not make read request; error = %d\n", stat);
      return 1;
    }

    memcpy(data + nbulk * size, buffer, remains * 4);
  }

  free(buffer);
  
  return 0;
}

unsigned int TCBReadReg(int sid, uint32_t mid, uint32_t addr)
{
  unsigned char data[4];
  unsigned int value;
  unsigned int tmp;

  TCBRead(sid, mid, 1, addr, data);

  value = data[0] & 0xFF;
  tmp = data[1] & 0xFF;
  value = value + (unsigned int)(tmp << 8);
  tmp = data[2] & 0xFF;
  value = value + (unsigned int)(tmp << 16);
  tmp = data[3] & 0xFF;
  value = value + (unsigned int)(tmp << 24);

  return value;
}  



// ******************************************************************************************************

// initialize libusb library
void USB3Init(void)
{
  if (libusb_init(0) < 0) {
    fprintf(stderr, "Failed to initialise LIBUSB\n");
    exit(1);
  }
}

// de-initialize libusb library
void USB3Exit(void)
{
  libusb_exit(0); 
}

// open CALTCB
int CALTCBopen(int sid)
{
  struct libusb_device **devs;
  struct libusb_device *dev;
  struct libusb_device_handle *devh;
  size_t i = 0;
  int nopen_devices = 0; //number of open devices
  int r;
  int interface = 0;
  int sid_tmp;
  int speed;

  if (libusb_get_device_list(0, &devs) < 0) 
    fprintf(stderr, "Error: open_device: Could not get device list\n");

  fprintf(stdout, "Info: open_device: opening device Vendor ID = 0x%X, Product ID = 0x%X, Serial ID = %u\n",
                   CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid);

  while ((dev = devs[i++])) {
    struct libusb_device_descriptor desc;
    r = libusb_get_device_descriptor(dev, &desc);
    if (r < 0) {
      fprintf(stdout, "Warning, open_device: could not get device device descriptior." " Ignoring.\n");
      continue;
    }

    if (desc.idVendor == CALTCB_VENDOR_ID && desc.idProduct == CALTCB_PRODUCT_ID)  {
      r = libusb_open(dev, &devh);
      if (r < 0) {
        fprintf(stdout, "Warning, open_device: could not open device." " Ignoring.\n");
        continue;
      }

      // do not open twice
      if (is_device_open(devh)) {
        fprintf(stdout, "Info, open_device: device already open." " Ignoring.\n");
        libusb_close(devh);
        continue;
      }

      // See if sid matches
      // Assume interface 0
      if (libusb_claim_interface(devh, interface) < 0) {
        fprintf(stdout, "Warning, open_device: could not claim interface 0 on the device." " Ignoring.\n");
        libusb_close(devh);
        continue;
      }

      sid_tmp = get_serial_id(devh);

      if (sid == 0xFF || sid == sid_tmp) {
        add_device(&ldev_open, devh, CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid_tmp);
        nopen_devices++;
  
        // Print out the speed of just open device 
        speed = libusb_get_device_speed(dev);
        switch (speed) {
          case 4:
            fprintf(stdout, "Info: open_device: super speed device opened");
            break;
          case 3:
            fprintf(stdout, "Info: open_device: high speed device opened");
            break;
          case 2:
            fprintf(stdout, "Info: open_device: full speed device opened");
            break;
          case 1:
            fprintf(stdout, "Info: open_device: low speed device opened");
            break;
          case 0:
            fprintf(stdout, "Info: open_device: unknown speed device opened");
            break;
        }
        
        fprintf(stdout, " (bus = %d, address = %d, serial id = %u).\n",
                    libusb_get_bus_number(dev), libusb_get_device_address(dev), sid_tmp);
        libusb_release_interface(devh, interface);
        break;
      }
      else {
        libusb_release_interface(devh, interface);
        libusb_close(devh);
      }
    }
  }

  libusb_free_device_list(devs, 1);

  // claim interface
  handle_interface_id(&ldev_open, CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid, 0, kInterfaceClaim);

  if (!nopen_devices)
    return -1;

  devh = nkusb_get_device_handle(CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid);
  if (!devh) {
    fprintf(stderr, "Could not get device handle for the device.\n");
    return -1;
  }

  return 0;
}

// close CALTCB
void CALTCBclose(int sid)
{
  handle_interface_id(&ldev_open, CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid, 0, kInterfaceRelease);
  remove_device_id(&ldev_open, CALTCB_VENDOR_ID, CALTCB_PRODUCT_ID, sid);
}

// reset timer 
void CALTCBresetTIMER(int sid)
{
  TCBWrite(sid, 0, 0x0, 0x01);
}

// reset 
void CALTCBreset(int sid)
{
  TCBWrite(sid, 0, 0x0, 0x02);
}

// start DAQ
void CALTCBstart_DAQ(int sid)
{
  TCBWrite(sid, 0, 0x0, 0x04);
}

// stop DAQ
void CALTCBstop_DAQ(int sid)
{
  TCBWrite(sid, 0, 0x0, 0x08);
}

// start DRS
void CALTCBstart_DRS(int sid)
{
  TCBWrite(sid, 0, 0x0, 0x10);
}

// stop DRS
void CALTCBstop_DRS(int sid)
{
  TCBWrite(sid, 0, 0x0, 0x20);
}

// read DAQ status
unsigned long CALTCBread_RUN(int sid, unsigned long mid)
{
  return TCBReadReg(sid, mid, 0x0);
}

// read DAQ link status
void CALTCBread_LINK(int sid, unsigned long *data)
{
  data[0] = TCBReadReg(sid, 0, 0x1);
  data[1] = TCBReadReg(sid, 0, 0x2);
}

// read mids ; TCB
void CALTCBread_MID(int sid, unsigned long *data)
{
  int i;

  for (i = 0; i < 40; i ++) 
    data[i] = TCBReadReg(sid, 0, 0x3 + i);
}

// write coincidence width, 0~15 * 1000 / 90 ns
void CALTCBwrite_CW(int sid, unsigned long mid, unsigned long data)
{
  unsigned long addr;
  
  if (mid == 0)
    addr = 0x2B;
  else
    addr = 0x01;

  TCBWrite(sid, mid, addr, data);
}

// read coincidence width
unsigned long CALTCBread_CW(int sid, unsigned long mid)
{
  unsigned long addr;
  
  if (mid == 0)
    addr = 0x2B;
  else
    addr = 0x01;

  return TCBReadReg(sid, mid, addr);
}

// write run number
void CALTCBwrite_RUN_NUMBER(int sid, unsigned long data)
{
  TCBWrite(sid, 0, 0x2C, data);
}

// read run number
unsigned long CALTCBread_RUN_NUMBER(int sid)
{
  return TCBReadReg(sid, 0, 0x2C);
}

// send software trigger
void CALTCBsend_TRIG(int sid)
{
  TCBWrite(sid, 0, 0x2D, 0x0);
}

// write pedestal trigger interval, 0 ~ 65535 ms, when 0 disabled
void CALTCBwrite_PEDESTAL_TRIGGER_INTERVAL(int sid, unsigned long data)
{
  TCBWrite(sid, 0, 0x2E, data);
}

// read pedestal trigger interval
unsigned long CALTCBread_PEDESTAL_TRIGGER_INTERVAL(int sid)
{
  return TCBReadReg(sid, 0, 0x2E);
}

// write trigger enable, self = 1, pedestal = 2, software = 4, external = 8
//                       external & software & pedestal & self
void CALTCBwrite_TRIGGER_ENABLE(int sid, unsigned long data)
{
  TCBWrite(sid, 0, 0x2F, data);
}

// read trigger enable
unsigned long CALTCBread_TRIGGER_ENABLE(int sid)
{
  return TCBReadReg(sid, 0, 0x2F);
}

// write multiplicity threshold, 1 ~ 2047
void CALTCBwrite_MULTIPLICITY_THR(int sid, unsigned long mid, unsigned long data)
{
  TCBWrite(sid, mid, 0x30, data);
}

// read multiplicity threshold
unsigned long CALTCBread_MULTIPLICITY_THR(int sid, unsigned long mid)
{
  return TCBReadReg(sid, mid, 0x30);
}

// write trigger delay, 0~15 * 1000 / 90 ns
void CALTCBwrite_TRIGGER_DELAY(int sid, unsigned long data)
{
	printf("?? test sk");
  TCBWrite(sid, 0, 0x31, data);
}

// read trigger delay
unsigned long CALTCBread_TRIGGER_DELAY(int sid)
{
  return TCBReadReg(sid, 0, 0x31);
}

// write high voltage, 0 ~ 60 V, ch = 1 ~ 4 (DRS chip #)
void CALTCBwrite_HV(int sid, unsigned long mid, unsigned long ch, float data)
{
  float fval;
  int value;
  unsigned long addr = 0x02;
  addr += ((ch - 1) & 0xFF) << 8;

  fval = 6.25 * (data - 4.5);
  value = (int)(fval);
  if (value > 254)
    value = 254;
  else if (value < 0)
    value = 0;

  TCBWrite(sid, mid, addr, value);
}

// read high voltage
float CALTCBread_HV(int sid, unsigned long mid, unsigned long ch)
{
  unsigned long data;
  float value;
  unsigned long addr = 0x02;
  addr += ((ch - 1) & 0xFF) << 8;

  data = TCBReadReg(sid, mid, addr);
  value = data;
  value = value / 6.25 + 4.5;

  return value;
}

// write threshold, 1 ~ 4095, 0.5 ~ 2047.5 mV
void CALTCBwrite_THR(int sid, unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long addr = 0x03;
  addr += ((ch - 1) & 0xFF) << 8;

  TCBWrite(sid, mid, addr, data);
}

// read threshold
unsigned long CALTCBread_THR(int sid, unsigned long mid, unsigned long ch)
{
  unsigned long addr = 0x03;
  addr += ((ch - 1) & 0xFF) << 8;

  return TCBReadReg(sid, mid, addr);
}

// read temperature
float CALTCBread_TEMP(int sid, unsigned long mid)
{
  unsigned long data;
  float fval;

  data = TCBReadReg(sid, mid, 0x04);
  fval = (data - 2048) * 0.0625;

  return fval;
}

// read DRS locked
unsigned long CALTCBread_DRS_PLL_LOCKED(int sid, unsigned long mid)
{
  return TCBReadReg(sid, mid, 0x05);
}

// read threshold
unsigned long CALTCBread_DRAMTEST(int sid, unsigned long mid, unsigned long ch)
{
  unsigned long addr = 0x07;
  addr += (ch & 0xFF) << 8;
  
  return TCBReadReg(sid, mid, addr);
}


// write DRS calibration, 1 = calibration, 0 = normal
void CALTCBwrite_DRS_CALIB(int sid, unsigned long mid, unsigned long data)
{
  TCBWrite(sid, mid, 0x21, data);
}

void CALTCBwrite_DRS_OFS(int sid, unsigned long mid, unsigned long rofs, unsigned long oofs)
{
  unsigned long data;

  data = (rofs & 0xFFFF) + ((oofs & 0xFFFF) << 16);

  TCBWrite(sid, mid, 0x20, data);
}

void CALTCBwrite_ADC_SETUP(int sid, unsigned long mid, unsigned long addr, unsigned long data)
{
  unsigned long wdata;

  wdata = (data & 0xFF) + ((addr & 0xFF) << 8);

  TCBWrite(sid, mid, 0x22, wdata);
}

// turn on DRAM
void CALTCBsetup_DRAM(int sid, unsigned long mid)
{
  unsigned long status;

  // check DRAM is on
  status = TCBReadReg(sid, mid, 0x06);
  
  // when DRAM is on now, turn it off
  if (status) 
    TCBWrite(sid, mid, 0x06, 0);

  // turn on DRAM
  TCBWrite(sid, mid, 0x06, 1);

  // wait for DRAM ready
  status = 0;
  while (!status) 
    status = TCBReadReg(sid, mid, 0x06);
}

// write DRAM test mode
void CALTCBwrite_DRAMTEST(int sid, unsigned long mid, int data)
{
  TCBWrite(sid, mid, 0x07, data);
}

// read DRAM alignment
int CALTCBread_DRAM_ALIGN(int sid, unsigned long mid, int ch)
{
  int addr;

  addr = 0x07 + ((ch & 0xFF) << 8);
  return TCBReadReg(sid, mid, addr);
}

// reset delay reference clock
void CALTCBreset_REF_CLK(int sid, unsigned long mid)
{
  TCBWrite(sid, mid, 0x23, 0);
}

// write DRAM input delay
void CALTCBwrite_DRAMDLY(int sid, unsigned long mid, int ch, int data)
{
  int addr;

  addr = 0x24 + ((ch & 0xFF) << 8);
  TCBWrite(sid, mid, addr, data);
}

// write DRAM input bitslip
void CALTCBwrite_DRAM_BITSLIP(int sid, unsigned long mid, int ch)
{
  int addr;

  addr = 0x25 + ((ch & 0xFF) << 8);
  TCBWrite(sid, mid, addr, 0);
}

// align DRAM
void CALTCBalign_DRAM(int sid, unsigned long mid)
{
  int ch;
  int dly;
  int value;
  int flag;
  int count;
  int sum;
  int aflag;
  int gdly;
  int bitslip;
  int gbitslip;

  // turn on DRAM    
  CALTCBsetup_DRAM(sid, mid);

  // enter DRAM test mode
  CALTCBwrite_DRAMTEST(sid, mid, 1);

  // send reset to iodelay  
  CALTCBreset_REF_CLK(sid, mid);

  // fill DRAM test pattern
  CALTCBwrite_DRAMTEST(sid, mid, 2);

  for (ch = 0; ch < 2; ch ++) {
    count = 0;
    sum = 0;
    flag = 0;

    // search delay
    for (dly = 0; dly < 32; dly++) {
      // set delay
      CALTCBwrite_DRAMDLY(sid, mid, ch, dly);

      // read DRAM test pattern
      CALTCBwrite_DRAMTEST(sid, mid, 3);
      value = CALTCBread_DRAMTEST(sid, mid, ch);
printf("ch = %d, dly = %d, value = %X\n", ch, dly, value);

      aflag = 0;
      if (value == 0xFFAA5500)
        aflag = 1;
      else if (value == 0xAA5500FF)
        aflag = 1;
      else if (value == 0x5500FFAA)
        aflag = 1;
      else if (value == 0x00FFAA55)
        aflag = 1;
    
      if (aflag) {
        count = count + 1;
        sum = sum + dly;
        if (count > 8)
          flag = 1; 
     }
      else {
        if (flag)
          dly = 32;
        else {
          count = 0;
          sum = 0;
        }
      }
    }

    // get bad delay center
    if (count)
      gdly = sum / count;
    else
      gdly = 9;

    // set delay
    CALTCBwrite_DRAMDLY(sid, mid, ch, gdly);
  
    // get bitslip
    for (bitslip = 0; bitslip < 4; bitslip++) {
      // read DRAM test pattern
      CALTCBwrite_DRAMTEST(sid, mid, 3);
      value = CALTCBread_DRAMTEST(sid, mid, ch);

      if (value == 0xFFAA5500) {
        aflag = 1;
        gbitslip = bitslip;
        bitslip = 4;
      }
      else {
        aflag = 0;
        CALTCBwrite_DRAM_BITSLIP(sid, mid, ch);
      }
    }

    if (aflag) 
      printf("DRAM(%d) is aligned, delay = %d, bitslip = %d\n", ch, gdly, gbitslip);
    else 
      printf("Fail to align DRAM(%d)!\n", ch);
  }

  // exit DRAM test mode
  CALTCBwrite_DRAMTEST(sid, mid, 0);
}

// initialize DRS
int CALTCB_DRSinit(int sid, unsigned long mid)
{
  // set ADC
  CALTCBwrite_ADC_SETUP(sid, mid, 0x0D, 0x00);
  CALTCBwrite_ADC_SETUP(sid, mid, 0xFF, 0x01);

  // DRS rofs to 3100 (~ 1.56 V)
  // DRS o-ofs to 2400 (~ 1.17 V)
  // do twice because DAC mux is not well defined for the first writing  
  CALTCBwrite_DRS_OFS(sid, mid, 3075, 2400);

  // stop DRS and restart it
  CALTCBstop_DRS(sid);
  usleep(100000);
  CALTCBstart_DRS(sid);
  usleep(100000);

  // align DRAM input
  CALTCBalign_DRAM(sid, mid);

  // check DRS PLL locked
  printf("DRS PLL lock status = %ld\n", CALTCBread_DRS_PLL_LOCKED(sid, mid));
 
  // calibration off
  CALTCBwrite_DRS_CALIB(sid, mid, 0);

  return 1;
}


