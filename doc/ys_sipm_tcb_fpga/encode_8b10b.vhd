library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
library UNISIM;
use UNISIM.VComponents.all;
use work.ys_sipm_tcb_fpga_type.all;

entity encoder_8b10b is port(
	sdout : in std_logic_vector(7 downto 0);
	charisk : in std_logic;
	disparity : in std_logic;
	clk : in std_logic;
	parout : out std_logic_vector(9 downto 0);
	dispp : out std_logic;
	dispm : out std_logic
); end encoder_8b10b;

architecture Behavioral of encoder_8b10b is

signal ra : std_logic_vector(13 downto 0);
signal rd : std_logic_vector(15 downto 0);

begin

	ra <= charisk & disparity & sdout(7 downto 0) & "0000";

	RAMB18E1_encoder_8b10b : RAMB18E1
	generic map (
		INIT_00 => X"14BA5B4E5B4D5B6C5B4B5B6A5B6914A75B475B665B6514AB5B6314AD14AE14B9",
		INIT_01 => X"14B5149E149D5B5C149B5B5A5B5914B314975B565B555B745B535B725B7114B6",
		INIT_02 => X"5A7A164E164D166C164B166A16695A671647166616655A6B16635A6D5A6E5A79",
		INIT_03 => X"5A755A5E5A5D165C5A5B165A16595A735A571656165516741653167216715A76",
		INIT_04 => X"5ABA168E168D16AC168B16AA16A95AA7168716A616A55AAB16A35AAD5AAE5AB9",
		INIT_05 => X"5AB55A9E5A9D169C5A9B169A16995AB35A971696169516B4169316B216B15AB6",
		INIT_06 => X"5B3A14CE14CD14EC14CB14EA14E95B2714C714E614E55B2B14E35B2D5B2E5B39",
		INIT_07 => X"5B355B1E5B1D14DC5B1B14DA14D95B335B1714D614D514F414D314F214F15B36",
		INIT_08 => X"153A5ACE5ACD5AEC5ACB5AEA5AE915275AC75AE65AE5152B5AE3152D152E1539",
		INIT_09 => X"1535151E151D5ADC151B5ADA5AD9153315175AD65AD55AF45AD35AF25AF11536",
		INIT_0A => X"597A154E154D156C154B156A15695967154715661565596B1563596D596E5979",
		INIT_0B => X"5975595E595D155C595B155A1559597359571556155515741553157215715976",
		INIT_0C => X"59BA158E158D15AC158B15AA15A959A7158715A615A559AB15A359AD59AE59B9",
		INIT_0D => X"59B5599E599D159C599B159A159959B359971596159515B4159315B215B159B6",
		INIT_0E => X"163A59CE59CD59EC59CB59EA59E9162759C759E659E5162B59E3162D162E1639",
		INIT_0F => X"1635161E161D59DC161B59DA59D91633161759D659D55BB459D35BB25BB11636",
		INIT_10 => X"1745908E908D90AC908B90AA90A9175890B890A690A5175490A3175217511746",
		INIT_11 => X"174A17611762909C1764909A9099174C17689096909590B4909390B290B11749",
		INIT_12 => X"9245164E164D166C164B166A1669925816781666166592541663925292519246",
		INIT_13 => X"924A92619262165C9264165A1659924C92681656165516741653167216719249",
		INIT_14 => X"9285168E168D16AC168B16AA16A9929816B816A616A5929416A3929292919286",
		INIT_15 => X"928A92A192A2169C92A4169A1699928C92A81696169516B4169316B216B19289",
		INIT_16 => X"90C5170E170D172C170B172A172990D817381726172590D4172390D290D190C6",
		INIT_17 => X"90CA90E190E2171C90E4171A171990CC90E817161715173417131732173190C9",
		INIT_18 => X"16C5910E910D912C910B912A912916D891389126912516D4912316D216D116C6",
		INIT_19 => X"16CA16E116E2911C16E4911A911916CC16E891169115913491139132913116C9",
		INIT_1A => X"9145154E154D156C154B156A1569915815781566156591541563915291519146",
		INIT_1B => X"914A91619162155C9164155A1559914C91681556155515741553157215719149",
		INIT_1C => X"9185158E158D15AC158B15AA15A9919815B815A615A5919415A3919291919186",
		INIT_1D => X"918A91A191A2159C91A4159A1599918C91A81596159515B4159315B215B19189",
		INIT_1E => X"15C5904E904D922C904B922A922915D892389226922515D4922315D215D115C6",
		INIT_1F => X"15CA15E115E2921C15E4921A921915CC15E892169215923492139232923115C9",
		INIT_20 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_21 => X"00000000000014BC000000000000000000000000000000000000000000000000",
		INIT_22 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_23 => X"0000000000005A7C000000000000000000000000000000000000000000000000",
		INIT_24 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_25 => X"0000000000005ABC000000000000000000000000000000000000000000000000",
		INIT_26 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_27 => X"0000000000005B3C000000000000000000000000000000000000000000000000",
		INIT_28 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_29 => X"000000000000153C000000000000000000000000000000000000000000000000",
		INIT_2A => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_2B => X"000000000000597C000000000000000000000000000000000000000000000000",
		INIT_2C => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_2D => X"00000000000059BC000000000000000000000000000000000000000000000000",
		INIT_2E => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_2F => X"0000145E145D147C145B00000000000014570000000000000000000000000000",
		INIT_30 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_31 => X"0000000000001743000000000000000000000000000000000000000000000000",
		INIT_32 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_33 => X"0000000000009183000000000000000000000000000000000000000000000000",
		INIT_34 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_35 => X"0000000000009143000000000000000000000000000000000000000000000000",
		INIT_36 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_37 => X"00000000000090C3000000000000000000000000000000000000000000000000",
		INIT_38 => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_39 => X"00000000000016C3000000000000000000000000000000000000000000000000",
		INIT_3A => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_3B => X"0000000000009283000000000000000000000000000000000000000000000000",
		INIT_3C => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_3D => X"0000000000009243000000000000000000000000000000000000000000000000",
		INIT_3E => X"0000000000000000000000000000000000000000000000000000000000000000",
		INIT_3F => X"000017A117A2178317A400000000000017A80000000000000000000000000000",
		READ_WIDTH_A => 18, 
		READ_WIDTH_B => 18, 
		WRITE_WIDTH_A => 18,
		WRITE_WIDTH_B => 18,
		SIM_DEVICE => "7SERIES"
	)
	port map (
		DOADO => rd,
		DOPADOP => open,
		DOBDO => open,
		DOPBDOP => open,
		ADDRARDADDR => ra, 
		CLKARDCLK => clk,
		ENARDEN => '1',
		REGCEAREGCE => '1',
		RSTRAMARSTRAM => '0',
		RSTREGARSTREG => '0',
		WEA => "00",
		DIADI => "1111111111111111",
		DIPADIP => "11",
		ADDRBWRADDR => "11111111111111",
		CLKBWRCLK => clk,
		ENBWREN => '1',
		REGCEB => '1',
		RSTRAMB => '0',
		RSTREGB => '0',
		WEBWE => "0000",
		DIBDI => "1111111111111111",
		DIPBDIP => "11"
	);

	parout <= rd(9 downto 0);
	dispp <= rd(14);
	dispm <= rd(15);

end Behavioral;

