--------------------------------------------------------------------------------------------------------------------
-- Design Name    : Synchroniser for Clock Domain Crossing   
-- Description    : Configurable no. of flip-flops in the synchroniser chain         
-- Date           : 05-07-2019
-- Designed By    : Mitu Raj, iammituraj@gmail.com
-- Comments       : Attributes are important for proper implementation, cross check synthesised design
--------------------------------------------------------------------------------------------------------------------

--------------------------------------------------------------------------------------------------------------------
-- LIBRARIES
--------------------------------------------------------------------------------------------------------------------
Library IEEE;
use IEEE.STD_LOGIC_1164.all;

--------------------------------------------------------------------------------------------------------------------
-- ENTITY DECLARATION
--------------------------------------------------------------------------------------------------------------------
Entity synchronizer is
    Generic (stages : natural := 2);     -- Recommended 2 flip-flops for low speed designs; >2 for high speed
    Port ( 
          clk           : in std_logic;
          async_sig_i   : in std_logic;
          sync_sig_o    : out std_logic
          );
end synchronizer;

--==================================================================================================================
-- ARCHITECTURE DEFINITION
--==================================================================================================================
Architecture Behavioral of synchronizer is

--------------------------------------------------------------------------------------------------------------------
-- Synchronisation Chain of Flip-Flops
--------------------------------------------------------------------------------------------------------------------
signal flipflops : std_logic_vector(stages-1 downto 0) := (others => '0');
--------------------------------------------------------------------------------------------------------------------

--------------------------------------------------------------------------------------------------------------------
-- These attributes are native to XST and Vivado Synthesisers.
-- They make sure that the synchronisers are not optimised to shift register primitives.
-- They are correctly implemented in the FPGA, by placing them together in the same slice.
-- Maximise MTBF while place and route.
-- Altera has different attributes.
--------------------------------------------------------------------------------------------------------------------
attribute ASYNC_REG : string;
attribute ASYNC_REG of flipflops: signal is "true";
--------------------------------------------------------------------------------------------------------------------

begin

   sync_sig_o <= flipflops(flipflops'high);  -- Synchronised signal out

   -- Synchroniser process
   clk_proc: process(clk)
             begin
                if rising_edge(clk) then
                   flipflops <= flipflops(flipflops'high-1 downto 0) & async_sig_i;
                end if;
             end process;

end Behavioral;
--------------------------------------------------------------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------
