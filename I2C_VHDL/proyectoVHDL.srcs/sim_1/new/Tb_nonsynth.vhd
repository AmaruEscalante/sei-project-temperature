--%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
--- Design         : TEST BENCH FOR I2C MASTER IP CORE v.1.0
--- Description    : This testbench writes data to an i2c slave that consists of four registers and then reads back data from it.
---                  An i2c Slave is used for testing purpose. i2c slave is by default configured for 100 MHz clock and 0x3c address.
--- Developer      : Mitu Raj
--- Date Modified  : 22-08-2019
--- Contact        : iammituraj@gmail.com
--%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

--=========================================================================================================================================
-- LIBRARIES
--=========================================================================================================================================
Library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--=========================================================================================================================================
-- TEST BENCH DECLARATION
--=========================================================================================================================================
Entity Tb_nonsynth is
end Entity;

--=========================================================================================================================================
-- ARCHITECTURE DEFINITION
--=========================================================================================================================================
Architecture behav of Tb_nonsynth is

-----------------------------------------------------------------------------------------------------
-- TEST PARAMETERS
-- Configure the test parameters here, to test the i2c Master IP
-----------------------------------------------------------------------------------------------------
constant TB_MODE      : std_logic := '0';
constant SLV_ADDR     : std_logic_vector(6 downto 0) := "1001000";       -- '3C' in 7-bit notation
constant SLV_ADDR_WR  : std_logic_vector(7 downto 0) := SLV_ADDR & '0';  -- Write address
constant SLV_ADDR_RD  : std_logic_vector(7 downto 0) := SLV_ADDR & '1';  -- Read address
-----------------------------------------------------------------------------------------------------

-- Simulation specific constants
constant clk_period   : time    := 10 ns;                     -- clk_period in ns
constant PRD          : integer := 10;                        -- clk_period in integer format
constant N_RST        : integer := 10;                        -- 10 reset cycles 
constant RST_CYCLES   : time    := (N_RST*PRD)*1 ns;          -- Corresponding clk cycles no.

-- All internal signals and registers
signal clk           : std_logic;
signal i2c_soft_rst  : std_logic;
signal mode          : std_logic;
signal start         : std_logic;
signal stop          : std_logic;
signal wr            : std_logic;
signal rd            : std_logic;
signal en_ack        : std_logic;
signal ack           : std_logic;
signal intr          : std_logic;
signal data_in       : std_logic_vector(7 downto 0);
signal data_out      : std_logic_vector(7 downto 0);
signal sda           : std_logic;
signal sclk          : std_logic;
signal rst_slv       : std_logic;

-- Test status 
signal status        : std_logic := '1';  -- Acknowledgement status
signal validate      : std_logic := '1';  -- Data read-back validation status

-- i2c Slave specifics
signal myReg0        : std_logic_vector(7 downto 0);

-------------------------------------------------------------------------------------------------------------------------------------------
-- Component Declaration - DUT
-------------------------------------------------------------------------------------------------------------------------------------------
Component my_i2c_master
Port (
    -- Global signals
    clk       : in std_logic;                      -- Clock in
    rstn      : in std_logic;                      -- Active-low reset
    -- Control signals
    mode_in   : in std_logic;                      -- To choose fast/slow mode: '0' for slow, '1' for fast
    start_in  : in std_logic;                      -- To start/repeated start a transaction
    stop_in   : in std_logic;                      -- To stop the ongoing transaction
    wr_in     : in std_logic;                      -- To initiate write transaction
    rd_in     : in std_logic;                      -- To initiate read transaction
    en_ack_in : in std_logic;                      -- To enable acknowledge to slave
    -- Status signals
    ack_out   : out std_logic;                     -- Indicates whether slave acknowledged or not
    intr_out  : out std_logic;                     -- Interrupt signal
    -- Parallel data
    data_in   : in  std_logic_vector(7 downto 0);  -- Parallel data in  (for write)
    data_out  : out std_logic_vector(7 downto 0);  -- Parallel data out (for read)
    -- Serial interface
    sda       : inout std_logic;                   -- Serial data out
    sclk      : out std_logic                      -- Serial clock out
    );
end Component;

-------------------------------------------------------------------------------------------------------------------------------------------
-- Component Declaration - i2c Slave - Register Bank that contains four registers with first address 0x0
-------------------------------------------------------------------------------------------------------------------------------------------
Component i2cSlaveTop 
Port (
	clk    : in std_logic;
	rst    : in std_logic;
	sda    : inout std_logic;
	scl    : in std_logic;
	myReg0 : out std_logic_vector(7 downto 0)
	);
end Component;

-------------------------------------------------------------------------------------------------------------------------------------------
-- User Procedures
-- Emulates IP Driver APIs 
-------------------------------------------------------------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------------------
-- To reset the IP 
---------------------------------------------------------------------------------------------------
procedure i2c_rst (signal i2c_soft_rst : out std_logic) is
begin
    i2c_soft_rst <= '0';   -- Active-low reset assertion
    wait for RST_CYCLES;
    i2c_soft_rst <= '1';   -- De-assertion
end procedure;

---------------------------------------------------------------------------------------------------
-- To initially configure the IP ... sets the mode of i2c and resets the IP as well 
---------------------------------------------------------------------------------------------------
procedure i2c_init    (variable mode_i2c     : in std_logic;
                       signal i2c_soft_rst   : out std_logic;
                       signal mode           : out std_logic) is
begin
    mode <= mode_i2c;
    i2c_rst(i2c_soft_rst); 
end procedure;

---------------------------------------------------------------------------------------------------
-- To start a transaction in i2c bus 
---------------------------------------------------------------------------------------------------
procedure i2c_start    (constant SLV_ADDR_W_R : in std_logic_vector(7 downto 0);
                        signal data_in        : out std_logic_vector(7 downto 0);
                        signal start          : out std_logic;
                        signal status         : out std_logic) is
begin
    data_in <= SLV_ADDR_W_R;              -- Slave address + write/read bit  
    start   <= '1';                       -- Pulsing start
    wait until intr = '0';                -- Wait for interrupt to reset
    wait until clk'event and clk = '1'; 
    start <= '0';
    wait until intr = '1';                -- Wait for interrupt to set
    wait until clk'event and clk = '1'; 
    if ack = '1' then                     -- Checks for slave acknowledgement
       status <= '1';
    else
       status <= '0';
    end if;
end procedure;

---------------------------------------------------------------------------------------------------
-- To write a data in i2c bus 
---------------------------------------------------------------------------------------------------
procedure i2c_write    (variable data_i2c_in  : in std_logic_vector(7 downto 0);
                        signal data_in        : out std_logic_vector(7 downto 0);
                        signal wr             : out std_logic;
                        signal status         : out std_logic) is
begin
    data_in <= data_i2c_in ;             -- Write data
    wait until clk'event and clk = '1'; 
    wr <= '1';                           -- Assert write signal    
    wait until intr = '0';               -- Wait for interrupt to reset
    wait until clk'event and clk = '1'; 
    wr <= '0';                           -- De-assert write signal
    wait until intr = '1';               -- Wait for interrupt to set
    wait until clk'event and clk = '1'; 
    if ack = '1' then                    -- Checks for slave acknowledgement
       status <= '1';
    else
       status <= '0';
    end if;
end procedure;

---------------------------------------------------------------------------------------------------
-- To initiate repeated start in the transaction
---------------------------------------------------------------------------------------------------
procedure i2c_repstart (constant SLV_ADDR_W_R : in std_logic_vector(7 downto 0);
	                    signal data_in        : out std_logic_vector(7 downto 0);
	                    signal start          : out std_logic;
	                    signal status         : out std_logic) is
begin
    data_in <= SLV_ADDR_W_R;              -- Slave address + write/read bit   
    start   <= '1';                       -- Pulsing start
    wait until intr = '0';                -- Wait for interrupt to reset
    wait until clk'event and clk = '1'; 
    start <= '0';
    wait until intr = '1';                -- Wait for interrupt to set
    wait until clk'event and clk = '1'; 
    if ack = '1' then                     -- Checks for slave acknowledgement
       status <= '1';
    else
       status <= '0';
    end if;
end procedure;

---------------------------------------------------------------------------------------------------
-- To read a data in i2c bus with acknowledgement (ACK)
---------------------------------------------------------------------------------------------------
procedure i2c_readAck  (signal rd             : out std_logic;
	                    signal en_ack         : out std_logic;
	                    variable data_i2c_out : out bit_vector(7 downto 0)) is
begin    
    en_ack  <= '1';                          -- Enable master acknowledge
    rd      <= '1';                          -- Pulse read
    wait until intr = '0';                   -- Wait for interrupt to reset
    wait until clk'event and clk = '1'; 
    rd      <= '0';                         
    wait until intr = '1';                   -- Wait for interrupt to set
    wait until clk'event and clk = '1';
    data_i2c_out := to_bitvector(data_out);  -- To convert 'H' pull-up state to '1' in all bits
end procedure;

---------------------------------------------------------------------------------------------------
-- To read a data in i2c bus with no acknowledgement (NACK) 
---------------------------------------------------------------------------------------------------
procedure i2c_readNack  (signal rd             : out std_logic;
	                     signal en_ack         : out std_logic;
	                     variable data_i2c_out : out bit_vector(7 downto 0)) is
begin    
    en_ack  <= '0';                          -- Disable master acknowledge
    rd      <= '1';                          -- Pulse read
    wait until intr = '0';                   -- Wait for interrupt to reset
    wait until clk'event and clk = '1'; 
    rd      <= '0';                         
    wait until intr = '1';                   -- Wait for interrupt to set
    wait until clk'event and clk = '1';
    data_i2c_out := to_bitvector(data_out);  -- To convert 'H' pull-up state to '1' in all bits
end procedure;

---------------------------------------------------------------------------------------------------
-- To stop the transaction in i2c bus 
---------------------------------------------------------------------------------------------------
procedure i2c_stop     (signal stop : out std_logic) is
begin
    stop <= '1';                          -- Pulsing stop
    wait until intr = '0';                -- Wait for interrupt to reset
    wait until clk'event and clk = '1'; 
    stop <= '0';
    wait until intr = '1';                -- Wait for interrupt to set; end of transaction
    wait until clk'event and clk = '1';     
end procedure;

begin
---------------------------------------------------------------------------------------------------
-- DUT instantiation
---------------------------------------------------------------------------------------------------
dut : my_i2c_master port map (
	                          clk       => clk,
	                          rstn      => i2c_soft_rst,
	                          mode_in   => mode,
	                          start_in  => start,
                              stop_in   => stop,
	                          wr_in     => wr,
	                          rd_in     => rd,
	                          en_ack_in => en_ack,
	                          ack_out   => ack,
	                          intr_out  => intr,
	                          data_in   => data_in,
	                          data_out  => data_out,
	                          sda       => sda,
	                          sclk      => sclk
	                          );

---------------------------------------------------------------------------------------------------
-- i2c Slave instantiation -- 
---------------------------------------------------------------------------------------------------
slave : i2cSlaveTop port map (
	                          clk       => clk,
	                          rst       => rst_slv,                      
	                          sda       => sda,
	                          scl       => sclk,
	                          myReg0    => myReg0
	                          );

-------------------------------------------------------------------------------------------------------------------------------------------
-- TEST VECTORS PROCESS
-------------------------------------------------------------------------------------------------------------------------------------------
test_vectors : process
-- Variables
variable mode_i2c      : std_logic; 
variable data_i2c_in   : std_logic_vector(7 downto 0);
variable data_i2c_out  : bit_vector(7 downto 0);
variable dchk          : bit_vector(7 downto 0);
variable reg_addr      : std_logic_vector(7 downto 0) := "00000000";
begin
    -- Reset states
    wr     <= '0';
    rd     <= '0';
    start  <= '0';
    stop   <= '0';
    en_ack <= '0';

    report "%% TESTING I2C MASTER %%";
    
    -----------------------------------------------------------------------------------------------------------------
    -- Initializing IP
    -----------------------------------------------------------------------------------------------------------------
    i2c_rst(i2c_soft_rst);                        -- Reset the IP initially
    mode_i2c := TB_MODE;                          -- Set mode
    i2c_init(mode_i2c, i2c_soft_rst, mode);       -- Initialise the IP with given mode 

    -----------------------------------------------------------------------------------------------------------------
    -- Start transaction in the IP by writing slave address
    -----------------------------------------------------------------------------------------------------------------
    i2c_start(SLV_ADDR_WR, data_in, start, status);  -- Start transaction by writing slave address + write bit '0';   
    
    -----------------------------------------------------------------------------------------------------------------
    --Write register address to Slave
    -----------------------------------------------------------------------------------------------------------------
    i2c_write(reg_addr, data_in, wr, status);

    -----------------------------------------------------------------------------------------------------------------
    -- Writing some data to four registers continuously
    -----------------------------------------------------------------------------------------------------------------
    for i in 1 to 4 loop
       data_i2c_in := std_logic_vector(to_unsigned(i,8));
       i2c_write(data_i2c_in, data_in, wr, status);       
    end loop;    

    -------------------------------------------------------------------------------------------------------------
    -- Initiate repeated start to reset register address by writing it again
    -------------------------------------------------------------------------------------------------------------
    i2c_repstart(SLV_ADDR_WR, data_in, start, status);  -- Writing slave address + write bit '0'

    -----------------------------------------------------------------------------------------------------------------
    --Write register address to Slave
    -----------------------------------------------------------------------------------------------------------------
    i2c_write(reg_addr, data_in, wr, status);

    -------------------------------------------------------------------------------------------------------------
    -- Initiate repeated start to start reading from registers
    -------------------------------------------------------------------------------------------------------------
    i2c_repstart(SLV_ADDR_RD, data_in, start, status);  -- Writing slave address + read bit '1'

    -------------------------------------------------------------------------------------------------------------
    -- Start reading back from all registers
    -------------------------------------------------------------------------------------------------------------
    for i in 1 to 3 loop
        -- Read first 3 registers with master acknowledge
        i2c_readAck(rd, en_ack, data_i2c_out);   
        -- Data validation
        dchk := to_bitvector(std_logic_vector(to_unsigned(i, 8)));    
        if data_i2c_out = dchk then            
           validate <= '1';
        else
           validate <= '0';
        end if;
    end loop;

    ------------------------------------------------------------------------------------------------------------- 
    -- Stop reading by sending a NACK
    -------------------------------------------------------------------------------------------------------------
    -- Read last register with no master acknowledge
    i2c_readNack(rd, en_ack, data_i2c_out);
    -- Data validation
    dchk := to_bitvector(std_logic_vector(to_unsigned(4, 8)));    
    if data_i2c_out = dchk then            
        validate <= '1';
    else
        validate <= '0';
    end if;
    
    -----------------------------------------------------------------------------------------------------------------
    -- Stop transaction in the IP 
    -----------------------------------------------------------------------------------------------------------------
    i2c_stop(stop);                              -- Stop transaction                             

    report("Test successfully finished !! ... No errors found ...");

    wait;                                         -- Wait forever ... Simulation may be finished
end process;

rst_slv <= not i2c_soft_rst;

-------------------------------------------------------------------------------------------------------------------------------------------
-- CLOCKING PROCESS
-------------------------------------------------------------------------------------------------------------------------------------------
clock_process : process
begin
    clk <= '0';
    wait for clk_period/2;
    clk <= '1';
    wait for clk_period/2;
end process;

-- Adds weak pull-up on SDA line for simulation purpose
sda <= 'H';

-------------------------------------------------------------------------------------------------------------------------------------------
-- ALL ASSERTIONS IN THIS TEST
-------------------------------------------------------------------------------------------------------------------------------------------
Assert status = '1'   report "\nTest Failed @" & time'image(now) & " - No Acknowledgement Received" severity failure;
Assert validate = '1' report "\nTest Failed @" & time'image(now) & " - Data validation failed" severity failure; 
-------------------------------------------------------------------------------------------------------------------------------------------

end Architecture;

-------------------------------------------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------------------------------------
