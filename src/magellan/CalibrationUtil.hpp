#pragma once
#include <Arduino.h>
#include "MagellanParser.hpp"

class MagellanCalibrationUtil
{
public:
  /**
   * initialize the calibration utility
   * @param out the output stream to print calibration instructions to
   * @param magellan the MagellanParser instance to calibrate
   */
  MagellanCalibrationUtil(Print *out, MagellanParser *magellan)
  {
    this->out = out;
    this->magellan = magellan;
  }

  /**
   * update calibration process.
   * @note call this repeatedly in loop()
   * @note also call magellan.update() in loop(), preferably before calling this function
   * @note during calibration, consider the magellan state as invalid, even when ready() returns true
   */
  void update();

private:
  uint32_t last_print_millis = 0;

  /**
   * should print a message at most every interval milliseconds
   * @param interval the interval in milliseconds
   * @return true if a message should be printed
   */
  bool should_print(const uint32_t interval)
  {
    const uint32_t now = millis();
    if ((now - last_print_millis) > interval)
    {
      last_print_millis = now;
      return true;
    }

    return false;
  }

  /**
   * reset the print interval
   */
  void reset_should_print()
  {
    last_print_millis = 0;
  }

  /**
   * print the minimum and maximum values for all axes
   */
  void print_min_max();

private:
  enum state_t
  {
    WAIT_FOR_READY,         // wait for magellan to be ready
    PROMPT_SENSITIVITY_SET, // prompt user to set sensitivity
    WAIT_SENSITIVITY_SET,   // wait for user to set sensitivity to max (T=7 and R=7)
    PROMPT_MOVE,            // prompt user to move the space mouse to extremes
    WAIT_MOVE,              // wait for user to move the space mouse to extremes.
                            // while in this state, output the extremes regularly.
                            // move on when pressing the "*" button.
    FINISHED                // calibration is complete, output calibration values
  };

  state_t state = WAIT_FOR_READY;

  int16_t x_min = 0, x_max = 0,
          y_min = 0, y_max = 0,
          z_min = 0, z_max = 0,
          u_min = 0, u_max = 0,
          v_min = 0, v_max = 0,
          w_min = 0, w_max = 0;

private:
  MagellanParser *magellan;
  Print *out;
};
