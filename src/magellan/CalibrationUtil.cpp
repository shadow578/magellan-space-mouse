#include "CalibrationUtil.hpp"

void MagellanCalibrationUtil::update()
{
  switch (this->state)
  {
  case WAIT_FOR_READY:
  {
    // is ready?
    if (this->magellan->ready())
    {
      this->out->println(F("Starting calibration assistant...\nPlease follow the instructions carefully."));
      this->state = PROMPT_SENSITIVITY_SET;
      reset_should_print();
      break;
    }

    if (should_print(1000))
    {
      this->out->println(F("Waiting for Magellan to be ready."));
    }
    break;
  }
  case PROMPT_SENSITIVITY_SET:
  {
    this->out->println(F("Please set the sensitivity to the maximum level."));
    this->state = WAIT_SENSITIVITY_SET;
    break;
  }
  case WAIT_SENSITIVITY_SET:
  {
    // done already?
    if (this->magellan->get_translation_sensitivity() == 7 && this->magellan->get_rotation_sensitivity() == 7)
    {
      this->out->println(F("Sensitivity set to maximum level."));
      this->state = PROMPT_MOVE;
      reset_should_print();
      break;
    }

    // print every 1 second
    if (should_print(1000))
    {
      this->out->print(F("Current sensitivity: Translation="));
      this->out->print(this->magellan->get_translation_sensitivity());
      this->out->print(F(", Rotation="));
      this->out->println(this->magellan->get_rotation_sensitivity());
    }
    break;
  }
  case PROMPT_MOVE:
  {
    this->out->println(F("Please move the space mouse to the extremes of its range."));
    this->state = WAIT_MOVE;
    break;
  }
  case WAIT_MOVE:
  {
    // keep checking sensitivity
    if (this->magellan->get_translation_sensitivity() != 7 || this->magellan->get_rotation_sensitivity() != 7)
    {
      this->out->println(F("Sensitivity changed!"));
      this->state = PROMPT_SENSITIVITY_SET;
      reset_should_print();
      break;
    }

    // print every 1 second
    if (should_print(1000))
    {
      this->print_min_max();
    }

    // update minimum and maximum values for all 6 axes
    #define UPDATE_MINMAX(axis)                               \
      do                                                      \
      {                                                       \
        const int16_t n = this->magellan->get_##axis##_raw(); \
        if (n < this->axis##_min)                             \
        {                                                     \
          this->axis##_min = n;                               \
        }                                                     \
        if (n > this->axis##_max)                             \
        {                                                     \
          this->axis##_max = n;                               \
        }                                                     \
      } while (0);

    UPDATE_MINMAX(x);
    UPDATE_MINMAX(y);
    UPDATE_MINMAX(z);
    UPDATE_MINMAX(u);
    UPDATE_MINMAX(v);
    UPDATE_MINMAX(w);
    break;
  }
  default:
  {
    this->state = WAIT_FOR_READY;
  }
  }
}

void MagellanCalibrationUtil::print_min_max()
{
  #define PRINT_MINMAX(axis)                    \
    this->out->print(F("." STRINGIFY2(axis) "={")); \
    this->out->print(this->axis##_min);         \
    this->out->print(F(", "));                  \
    this->out->print(this->axis##_max);         \
    this->out->print(F("}, "));                 \
  
  this->out->print(F("Axis calibration: {"));
  PRINT_MINMAX(x);
  PRINT_MINMAX(y);
  PRINT_MINMAX(z);
  PRINT_MINMAX(u);
  PRINT_MINMAX(v);
  PRINT_MINMAX(w);
  this->out->println(F("}"));
}
