--%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
--- Design         : I2C MASTER IP CORE
--- Version        : v.1.0
--- Description    : I2C MASTER IP CORE 
---                  ## Features ###
---                  >> 8-bit data frame, 7-bit slave address
---                  >> Supports two modes of operation: Slow (sclk = 100 kHz), Fast (sclk = 400 kHz)
---                  >> Interrupt always enabled
---                  >> Pull-ups needed only on SDA line
---                  >> Only master can control SCLK
---                  >> MSb to LSb serial transmission order
---                  >> Clock is internally generated to drive logic, no PLLs
---                  >> Synchronisers are implemented for all control signals
--- Tested on      : Artix-7 FPGA, Xilinx Vivado 2015.4
---                  Timing successfully verified for 100 MHz core clock
---                  Optimal synthesis, No FPGA dedicated IPs used, bare RTL design              
--- Developer      : Mitu Raj
--- Date Modified  : 22-08-2019
--- Mail ID        : iammituraj@gmail.com
--- Link to Doc    : https://www.instructables.com/id/Design-of-I2C-Protocol-in-VHDL/
--- Copyright      : Open-Source License
--%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

--=========================================================================================================================================
-- LIBRARIES
--=========================================================================================================================================
Library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--=========================================================================================================================================
-- ENTITY DECLARATION
--=========================================================================================================================================
Entity my_i2c_master is 
Port (
    -- Global signals
    clk       : in std_logic;                       -- Clock in
    rstn      : in std_logic;                       -- Active-low reset
    -- Control signals
    mode_in   : in std_logic;                       -- To choose fast/slow mode: '0' for slow, '1' for fast
    start_in  : in std_logic;                       -- To start/repeated start a transaction
    stop_in   : in std_logic;                       -- To stop the ongoing transaction
    wr_in     : in std_logic;                       -- To initiate write transaction
    rd_in     : in std_logic;                       -- To initiate read transaction
    en_ack_in : in std_logic;                       -- To enable acknowledge to slave
    -- Status signals
    ack_out   : out std_logic;                      -- Indicates whether slave acknowledged or not
    intr_out  : out std_logic;                      -- Interrupt signal
    -- Parallel data
    data_in   : in  std_logic_vector(7 downto 0);   -- Parallel data in  (for write)
    data_out  : out std_logic_vector(7 downto 0);   -- Parallel data out (for read)
    -- Serial interface
    sda       : inout std_logic;                    -- Serial data out
    sclk      : out std_logic                       -- Serial clock out
    );
end Entity;

--=========================================================================================================================================
-- ARCHITECTURE DEFINITION
--=========================================================================================================================================
Architecture behav of my_i2c_master is

-- All constants
constant CLK_FRQ  : positive := 100000;             -- Main clock frequency in kHz ** Configure clock accordingly here **
constant CNT_SLOW : positive := CLK_FRQ/(200*2);    -- Clock division factor to generate 2*slow clock 100 kHz, from clk 100 MHz
constant CNT_FAST : positive := CLK_FRQ/(800*2);    -- Clock division factor to generate 2*fast clock 400 kHz, from clk 100 MHz

-- Internal signals
signal clk_i2c    : std_logic;                      -- Internally generated clock to drive the FSM
signal sda_x      : std_logic;                      -- Signal to control SDA line
-- Buffers
signal di_buffer  : std_logic_vector(7 downto 0);   -- To buffer data word in
signal do_buffer  : std_logic_vector(7 downto 0);   -- To buffer data word out
-- Other registers
signal state      : std_logic_vector(7 downto 0);   -- State register
signal count      : integer range 0 to 255;         -- Counter to generate internal I2C clock
signal bit_count  : integer range 0 to 7;           -- Counter to keep track of bit of a word

-- Synchronisation related signals
signal a_s_rstn   : std_logic;                      -- Synchronised reset
signal start      : std_logic;                      -- Synchronised start_in
signal stop       : std_logic;                      -- Synchronised stop_in
signal wr         : std_logic;                      -- Synchronised wr_in
signal rd         : std_logic;                      -- Synchronised rd_in
signal en_ack     : std_logic;                      -- Synchronised en_ack_in
signal ack        : std_logic;                      -- Non-synchronised ack out
signal intr       : std_logic;                      -- Non-synchronised intr out

-------------------------------------------------------------------------------------------------------------------------------------------
-- Async Reset Circuitry Declaration
-------------------------------------------------------------------------------------------------------------------------------------------
Component async_rst_ckt 
    Generic (stages : natural := 4);     
    Port ( 
          clk           : in std_logic;
          rstn          : in std_logic;
          async_rst_out : out std_logic
          );
end Component;

------------------------------------------------------------------------------------------------------------------------
-- Synchroniser Declaration
------------------------------------------------------------------------------------------------------------------------
Component synchronizer 
    Generic (stages : natural := 2);     -- Recommended 2 flip-flops for low speed designs; >2 for high speed
    Port ( 
          clk           : in std_logic;
          async_sig_i   : in std_logic;
          sync_sig_o    : out std_logic
          );
end Component;

begin   

-------------------------------------------------------------------------------------------------------------------------------------------
-- Async Reset Circuitry Instantiation
-------------------------------------------------------------------------------------------------------------------------------------------
inst_async_rst_ckt : async_rst_ckt port map (
                                        clk           => clk,
                                        rstn          => rstn,
                                        async_rst_out => a_s_rstn
                                        );

-------------------------------------------------------------------------------------------------------------------------------------------
-- Synchroniser Instantiations -- For all control signals crossing clock domains
-------------------------------------------------------------------------------------------------------------------------------------------

-- For start signal from clk --> clk_i2c
synch_start   : synchronizer port map (
                                   clk         => clk_i2c,
                                   async_sig_i => start_in,
                                   sync_sig_o  => start
                                   );

-- For stop signal from clk --> clk_i2c
synch_stop   : synchronizer port map (
                                   clk         => clk_i2c,
                                   async_sig_i => stop_in,
                                   sync_sig_o  => stop
                                   );

-- For write signal from clk --> clk_i2c
synch_wr      : synchronizer port map (
                                   clk         => clk_i2c,
                                   async_sig_i => wr_in,
                                   sync_sig_o  => wr
                                   );

-- For read signal from clk --> clk_i2c
synch_rd      : synchronizer port map (
                                   clk         => clk_i2c,
                                   async_sig_i => rd_in,
                                   sync_sig_o  => rd
                                   );

-- For enable acknowledge signal from clk --> clk_i2c
synch_en_ack  : synchronizer port map (
                                   clk         => clk_i2c,
                                   async_sig_i => en_ack_in,
                                   sync_sig_o  => en_ack
                                   );

-- For enable acknowledge signal from clk_i2c --> clk
synch_ack     : synchronizer port map (
                                   clk         => clk,
                                   async_sig_i => ack,
                                   sync_sig_o  => ack_out
                                   );

-- For enable acknowledge signal from clk_i2c --> clk
synch_intr    : synchronizer port map (
                                   clk         => clk,
                                   async_sig_i => intr,
                                   sync_sig_o  => intr_out
                                   );

-------------------------------------------------------------------------------------------------------------------------------------------
-- SDA LINE CONTROL
-------------------------------------------------------------------------------------------------------------------------------------------
sda <= 'Z' when sda_x = '1' else '0'; -- sda_x controls SDA line via tri-state buffer, 'Z' implies high cz of external pull-up resistors
-------------------------------------------------------------------------------------------------------------------------------------------

--=========================================================================================================================================
-- THE FSM FOR MASTER OPERATION
--=========================================================================================================================================
process(clk_i2c, a_s_rstn)
begin
      if a_s_rstn = '0' then
            -- Resetting ports
            data_out   <= (OTHERS => '0');
            ack        <= '0';
            intr       <= '1';     -- High by default
            sda_x      <= '1';     -- Idle state
            sclk       <= '1';     -- Idle state
            -- Resetting buffers
            di_buffer  <= (OTHERS => '0');   
            do_buffer  <= (OTHERS => '0');   
            -- Resetting internal signals and registers
            state      <= x"00";
            bit_count  <= 7;  
      elsif rising_edge(clk_i2c) then       
            case state is
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## IDLE STATE ##
                        -- Wait here for start signal with SDA and SCLK @high state
                        -------------------------------------------------------------------------------------------------------------------
                        when x"00"  =>    
                                    intr <= '1';            -- Default interrupt state                    
                                    if start = '1' then 
                                       sda_x <= '0';        -- START condition
                                       state <= x"01";
                                    else
                                       sda_x <= '1';
                                       state <= x"00";
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## INPUT DATA BUFFER STATE ##                        
                        -------------------------------------------------------------------------------------------------------------------
                        when x"01"  => 
                                    intr      <= '0';       -- De-assert interrupt, signifies that start condition has been completed                                       
                                    di_buffer <= data_in;   -- Buffer the input data 
                                    state     <= x"02"; 
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## BIT WRITE STATE ##  
                        -- Sends a data bit to SDA line 
                        -- First byte sent is always the 7-bit slave address + '0' or '1' bit (at LSB); '0' - write, '1' - read 
                        -- Reg address/Data bytes from second byte onwards                                                               
                        -------------------------------------------------------------------------------------------------------------------
                        when x"02"  =>
                                    sclk  <= '0';
                                    sda_x <= di_buffer(bit_count);   -- Writing bit to SDA line
                                    state <= x"03";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## BIT HOLD STATE ##  
                        -- Hold the data while SCLK is high                      
                        -------------------------------------------------------------------------------------------------------------------
                        when x"03"  =>
                                    sclk <= '1';
                                    if bit_count = 0 then
                                       bit_count <= 7;
                                       state     <= x"04";
                                    else
                                       bit_count <= bit_count - 1;
                                       state     <= x"02";
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## SDA RELEASE STATE ##                                             
                        -------------------------------------------------------------------------------------------------------------------
                        when x"04"  =>
                                    sclk  <= '0';  
                                    sda_x <= '1';   -- Master releases SDA line for Slave to acknowledge
                                    state <= x"05";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## SLAVE ACKNOWLEDGE STATE ##  
                        -- Checks for Slave's acknowledgement                      
                        ------------------------------------------------------------------------------------------------------------------- 
                        when x"05"  =>
                                    sclk <= '1';
                                    if sda = '0' then
                                       ack   <= '1';      -- Acknowledgement received !!                                                                     
                                    else
                                       ack   <= '0';      -- No acknowledgement received !! 
                                    end if;
                                    state <= x"06";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## INTERRUPT STATE ##  
                        -- Generate interrupt and waits here until moving to rep.start/read/write/stop state                  
                        -------------------------------------------------------------------------------------------------------------------
                        when x"06"  =>
                                    intr     <= '1';                             -- Assert interrupt
                                    -- Highest priority to 'stop'
                                    if stop = '1' then                           -- Proceed to stop the transaction
                                       state <= x"13";
                                    elsif start = '1' then   
                                       state <= x"16";                           -- Proceed to repeated start
                                    elsif di_buffer(0) = '0' and wr = '1' then   -- Proceed to write
                                       state <= x"07";         
                                    elsif di_buffer(0) = '1' and rd = '1' then   -- Proceed to read
                                       state <= x"0D";
                                    else                                         -- Stay here
                                       state <= x"06";
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## WRITE STATE - BUFFER INPUT DATA  ##                        
                        ------------------------------------------------------------------------------------------------------------------- 
                        when x"07"  =>      
                                    intr      <= '0';        -- De-assert interrupt                              
                                    di_buffer <= data_in;    -- Buffer data
                                    state     <= x"08"; 
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## BIT WRITE STATE ##
                        -------------------------------------------------------------------------------------------------------------------           
                        when x"08"  => 
                                    sclk  <= '0';
                                    sda_x <= di_buffer(bit_count);   
                                    state <= x"09";   
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## BIT HOLD STATE ##
                        -------------------------------------------------------------------------------------------------------------------              
                        when x"09"  =>
                                    sclk <= '1';
                                    if bit_count = 0 then
                                       bit_count <= 7;
                                       state     <= x"0A";
                                    else
                                       bit_count <= bit_count - 1;
                                       state     <= x"08";
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## SDA RELEASE STATE ##                                             
                        -------------------------------------------------------------------------------------------------------------------
                        when x"0A"  =>
                                    sclk  <= '0';  
                                    sda_x <= '1';            -- Master releases SDA line for Slave to acknowledge
                                    state <= x"0B";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## SLAVE ACKNOWLEDGE STATE ##                                            
                        ------------------------------------------------------------------------------------------------------------------- 
                        when x"0B"  =>
                                    sclk <= '1';
                                    if sda = '0' then
                                       ack <= '1';           -- Acknowledgement received !!                                       
                                    else
                                       ack <= '0';           -- No acknowledgement received !!                                       
                                    end if; 
                                    state <= x"0C";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## INTERRUPT STATE ##
                        -- Generates interrupt and waits here until moving to rep.start/stop/next write                               
                        -------------------------------------------------------------------------------------------------------------------
                        when x"0C"  =>
                                    intr <= '1';             -- Assert interrupt
                                    if stop = '1' then       -- Stop  
                                       state <= x"13";
                                    elsif start = '1' then      -- Repeated start
                                       state <= x"16";
                                    elsif wr = '1' then      -- Wait for write pulse to continue with write 
                                       state <= x"07";
                                    else                     -- Stay here
                                       state <= x"0C";
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## SLAVE WRITE STATE 1 ##   
                        -- Slave writes its first data bit                                           
                        -------------------------------------------------------------------------------------------------------------------                         
                        when x"0D"  =>   
                                    intr     <= '0';         -- De-assert interrupt                                   
                                    sclk     <= '0';     
                                    sda_x    <= '1';         -- Master releases SDA line, so that Slave can write
                                    state    <= x"0E";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## MASTER READ STATE ##   
                        -- Master samples data in SDA line                                          
                        -------------------------------------------------------------------------------------------------------------------            
                        when x"0E"  =>                                    
                                    sclk     <= '1';
                                    do_buffer(bit_count) <= sda;
                                    state    <= x"0F";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## SLAVE WRITE STATE 2 ##                                                                    
                        -------------------------------------------------------------------------------------------------------------------                
                        when x"0F"  =>
                                    sclk <= '0';
                                    if bit_count = 0 then
                                       bit_count <= 7;
                                       state     <= x"10";
                                    else
                                       bit_count <= bit_count - 1;
                                       state     <= x"0E";
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## MASTER ACKNOWLEDGE STATE ##
                        -- If acknowledge is enabled, Master will send ACK , else NACK                                                                   
                        -------------------------------------------------------------------------------------------------------------------
                        when x"10"  =>
                                    data_out <= do_buffer;   -- Buffer out the data
                                    if en_ack = '1' then
                                       sda_x <= '0';         -- ACK, continue reading
                                       state <= x"11";
                                    else
                                       sda_x <= '1';         -- NACK, can stop the read transaction
                                       state <= x"12";
                                    end if;                                    
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## INTERRUPT STATE - AFTER ACK ##                                                                                         
                        -------------------------------------------------------------------------------------------------------------------            
                        when x"11"  =>
                                    intr     <= '1';         -- Assert interrupt
                                    sclk     <= '1';         -- For slave to read acknowledgement
                                    if rd = '1' then         -- Wait for read pulse to continue with read
                                       state <= x"0D";
                                    else
                                       state <= x"11";       -- Stay here 
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## INTERRUPT STATE - AFTER NACK ## 
                        -- Waits here for stop/start signal                                                                                        
                        -------------------------------------------------------------------------------------------------------------------            
                        when x"12"  =>
                                    intr     <= '1';         -- Assert interrupt
                                    sclk     <= '1';         -- For slave to read acknowledgement
                                    if stop = '1' then
                                       state <= x"13";       -- Proceed to stop condition
                                    elsif start = '1' then
                                       state <= x"17";       -- Proceed to repeated start
                                    else
                                       state <= x"12";                                       
                                    end if;
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## STOP STATE 1 ##  
                        -- Pull SDA and SCLK low                 
                        -------------------------------------------------------------------------------------------------------------------    
                        when x"13"  =>
                                    intr     <= '0';         -- De-assert interrupt
                                    sda_x    <= '0';
                                    sclk     <= '0';
                                    state    <= x"14";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## STOP STATE 2 ##  
                        -- Pull SCLK high                 
                        -------------------------------------------------------------------------------------------------------------------
                        when x"14"  =>
                                    sclk  <= '1';
                                    state <= x"15";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## STOP STATE 3 ##  
                        -- Pull SDA high while SCLK is high, go back to IDLE state                
                        -------------------------------------------------------------------------------------------------------------------
                        when x"15"  =>
                                    sda_x <= '1';            -- STOP condition
                                    state <= x"00"; 
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## REPEATED START STATE 1 ##  
                        -- Pull SDA high and SCLK low                
                        -------------------------------------------------------------------------------------------------------------------
                        when x"16"  =>                                    
                                    sda_x <= '1';            
                                    sclk  <= '0';
                                    state <= x"17"; 
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## REPEATED START STATE 2 ##  
                        -- Pull SCLK high               
                        -------------------------------------------------------------------------------------------------------------------
                        when x"17"  =>
                                    sclk  <= '1';            
                                    state <= x"18";
                        -------------------------------------------------------------------------------------------------------------------
                        -- ## REPEATED START STATE 3 ##  
                        -- Pull SDA low while SCLK is high              
                        -------------------------------------------------------------------------------------------------------------------
                        when x"18"  =>
                                    sda_x <= '0';            
                                    state <= x"01";                                 
                        
                        when others =>    
            end case;        
      end if;
end process;

--=========================================================================================================================================
-- PROCESS TO GENERATE 2*SLOW/FAST CLOCK INTERNALLY
--=========================================================================================================================================
process(clk, a_s_rstn)
begin     
    if a_s_rstn ='0' then
        clk_i2c   <= '0';
        count     <= 0;
    elsif rising_edge(clk) then
        if mode_in = '0' then
            if count = CNT_SLOW then
               clk_i2c <= not clk_i2c;
               count   <= 1;
            else
               count   <= count + 1;
            end if;
        else
            if count = CNT_FAST then
               clk_i2c <= not clk_i2c;
               count   <= 1;
            else
               count   <= count + 1;
            end if; 
        end if;
    end if;  
end process;

end Architecture;

--%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%