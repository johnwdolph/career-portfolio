----------------------------------------------------------------------------
-- Description:  PWM Component
----------------------------------------------------------------------------
-- Author:       John Dolph
-- Company:      Montana State University
-- Create Date:  November 10, 2022
-- Revision:     1.0
----------------------------------------------------------------------------

-- Assigned Periods and Data Types

-- PWM Period: 38
-- Data Type -- period(W.F): 13.6
-- Data Type -- duty_cycle(W.F): 16.4

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;
use IEEE.std_logic_unsigned.all;
library altera;
use altera.altera_primitives_components.all;

entity myPWM is
    generic (
        W_period : integer := 13;
        F_period : integer := 6;
        W_duty_cycle : integer := 16;
        F_duty_cycle : integer := 4
    );
    port(
        clk : in std_logic; -- system clock
        reset : in std_logic; -- system reset
        SYS_CLKs_sec : in std_logic_vector(31 downto 0); -- system clocks in one secondecho "# career-portfolio" >> README.md
        period : in std_logic_vector(W_period-1 downto 0); -- PWM repetition period (milliseconds)
        duty_cycle : in std_logic_vector(W_duty_cycle-1 downto 0); -- PWM control word: [0 100];
        signal_out : out std_logic -- PWM output signal
    );
end entity;

architecture myPWM_arch of myPWM is

    -- constants for converting seconds to milliseconds and percent integer to decimal version
    constant const_bit_length : integer := 16; -- bit length for division (multiplication) conversion constants
    constant const_1_div_1000 : unsigned(const_bit_length-1 downto 0) := "0000000001000010"; -- 1000 (F=16)
    constant const_1_div_100 : unsigned(const_bit_length-1 downto 0) := "0000001010001111"; -- 100 (F=16)

    -- variables for determining increment fixed point location
    constant F_period_clocks : integer := F_period + const_bit_length;
    constant F_pulse_width_clocks : integer := F_period + F_duty_cycle + const_bit_length + const_bit_length;
    -- increment of 1 according to fixed point location of period_clocks and pulse_width_clocks
    constant period_increment : unsigned(SYS_CLKs_sec'length + W_period + const_bit_length - 1 downto 0) := (F_period_clocks => '1', others => '0'); -- same bit length as period_clocks
    constant pulse_width_increment : unsigned(SYS_CLKs_sec'length + W_period + W_duty_cycle + const_bit_length + const_bit_length - 1 downto 0) := (F_pulse_width_clocks => '1', others => '0');


    signal out_signal : std_logic := '0';

    -- period and pulse width in clocks so a counter may be made for the given specifications
    signal period_count : unsigned(SYS_CLKs_sec'length + W_period + const_bit_length - 1 downto 0) := (others => '0');
    signal period_clocks : unsigned(SYS_CLKs_sec'length + W_period + const_bit_length - 1 downto 0) := (others => '0');
    signal pulse_width_count : unsigned(SYS_CLKs_sec'length + W_period + W_duty_cycle + const_bit_length + const_bit_length - 1 downto 0) :=  (others => '0');
    signal pulse_width_clocks : unsigned(SYS_CLKs_sec'length + W_period + W_duty_cycle + const_bit_length + const_bit_length - 1 downto 0) := (others => '0');

    -- limitation on duty cycle range [0, 100]
    signal duty_cycle_limited : std_logic_vector(W_duty_cycle-1 downto 0);
    constant duty_limit : std_logic_vector(W_duty_cycle-1 downto 0) := "0000011001000000";

    -- limitation on period range [10ms, 50ms]
    signal period_limited : std_logic_vector(W_period-1 downto 0);
    constant period_lower_limit : std_logic_vector(W_period-1 downto 0) := "0001010000000"; -- 10ms
    constant period_upper_limit : std_logic_vector(W_period-1 downto 0) := "0110010000000"; -- 50ms

    begin

        -- route out_signal to the entity output
        signal_out <= out_signal;

        -- set proper limitations and determine clocks needed
        DUTY_SETUP : process(clk)
            begin
                if rising_edge(clk) then

                    -- limit range
                    if(unsigned(duty_cycle) > unsigned(duty_limit)) then
                        duty_cycle_limited <= duty_limit;
                    else
                        duty_cycle_limited <= duty_cycle;
                    end if;

                    --determine clock ount for one pulse width
                    pulse_width_clocks <= unsigned(period_limited) * unsigned(SYS_CLKs_sec) * unsigned(duty_cycle_limited) * const_1_div_1000 * const_1_div_100;
                end if;
        end process;

        -- set proper limitations and determine clocks needed
        PERIOD_SETUP : process(clk)
            begin
                if rising_edge(clk) then
                    
                    -- limit range
                    if(unsigned(period) > unsigned(period_upper_limit)) then
                        period_limited <= period_upper_limit;
                    elsif(unsigned(period) < unsigned(period_lower_limit)) then
                        period_limited <= period_lower_limit;
                    else
                        period_limited <= period;
                    end if;

                    --determine clock count for one period
                    period_clocks <= unsigned(period_limited) * unsigned(SYS_CLKs_sec) * const_1_div_1000;
                end if;
        end process;

        -- Behavioral model to output PWM signal
        PWM : process (clk)
            begin
                if rising_edge(clk) then
                    
                    if(reset = '1') then
                        out_signal <= '0';
                        period_count <= (others => '0');
                        pulse_width_count <= (others => '0');
                    else

                        if(out_signal = '1') then

                        -- signal goes low after each pulse width
                            if(pulse_width_count > pulse_width_clocks - pulse_width_increment or duty_cycle_limited = 0) then
                                out_signal <= '0';
                            end if;

                            pulse_width_count <= pulse_width_count + pulse_width_increment;
                            period_count <= period_count + period_increment;
                        else
                            -- signal goes high at each period = 0
                            if(period_count > period_clocks - period_increment or period_count = 0) then -- out signal low test
                                if(duty_cycle_limited > 0) then
                                    period_count <= (others => '0');
                                    pulse_width_count <= (others => '0');
                                    out_signal <= '1';
                                end if;
                            else
                                -- increment count each clock
                                period_count <= period_count + period_increment;
                            end if;
                        end if;

                    end if;
                end if;
        end process;

end architecture;
