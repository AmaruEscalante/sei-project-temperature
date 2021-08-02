--%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
--- Design         : ASYNC RESET CIRCUITRY
--- Version        : v.1.0
--- Description    : Circuit to generate asynchronous reset with safe synchronous de-assertion without any metastability issues         
--- Developer      : Mitu Raj
--- Date Modified  : 13-08-2019
--- Contact        : iammituraj@gmail.com
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
Entity async_rst_ckt is 
Generic (stages : natural := 4);       -- Decides min. reset pulse width
Port (    
    clk           : in std_logic;      -- Clock in
    rstn          : in std_logic;      -- Active-low async reset in 
    async_rst_out : out std_logic      -- Async reset with sync de-assertion out
    );
end Entity;

--=========================================================================================================================================
-- ARCHITECTURE DEFINITION
--=========================================================================================================================================
Architecture Behavioral of async_rst_ckt is

-------------------------------------------------------------------------------------------------------------------------------------------
-- Synchronised Chain of Flip-Flops
-------------------------------------------------------------------------------------------------------------------------------------------
signal flipflops : std_logic_vector(stages-1 downto 0) := (others => '0');
-------------------------------------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------------------------------------
-- These attributes are native to XST and Vivado Synthesisers.
-- They make sure that the synchronisers are not optimised to shift register primitives.
-- They are correctly implemented in the FPGA, by placing them together in the same slice.
-- Maximise MTBF while place and route.
-- Altera has different attributes.
-------------------------------------------------------------------------------------------------------------------------------------------
attribute ASYNC_REG : string;
attribute ASYNC_REG of flipflops: signal is "true";
-------------------------------------------------------------------------------------------------------------------------------------------

begin

   async_rst_out <= flipflops(flipflops'high);  -- Asynchronous reset with synchronised de-assertion out

   -- Synchroniser process
   clk_proc: process(clk, rstn)
             begin
                if rstn = '0' then
                   flipflops <= (others => '0');
                elsif rising_edge(clk) then
                   flipflops <= flipflops(flipflops'high-1 downto 0) & '1';
                end if;
             end process;

end Behavioral;

-------------------------------------------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------------------------------------