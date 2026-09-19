// Sega Genesis / Mega Drive controller
//
// DB9 pin -> function -> Arduino pin
//   1  Up      -> UP_PIN
//   2  Down    -> DOWN_PIN
//   3  Left    -> LEFT_PIN
//   4  Right   -> RIGHT_PIN
//   5  +5V     -> 5V
//   6  TL      -> TL_PIN   (reports A or B depending on TH)
//   7  TH      -> TH_PIN   (SELECT, driven by high/low determines multiplexing mode)
//   8  GND     -> GND
//   9  TR      -> TR_PIN   (reports Start or C depending on TH)

#include <Arduino.h>

#include "Gamepad.h"
#include "GenesisController.h"

#include "HID.h"
#include "HID-Settings.h"


namespace
{
  constexpr unsigned long PollInterval = 16;

  // Settle time after toggling TH before reading the data pins.
  // Must stay well under the controller's ~1.5ms "reset the state counter" window.
  constexpr unsigned int SettleTime = 20;

  Gamepad gamepad;
}

namespace Pin
{
  constexpr auto PowerLED = 0;

  constexpr auto TH = 2;
  constexpr auto Up = 3;
  constexpr auto Down = 4;
  constexpr auto Left = 5;
  constexpr auto Right = 6;
  constexpr auto TL = 7;
  constexpr auto TR = 8;
}


namespace Dpad
{
  constexpr auto Centered = 0;
  constexpr auto Up = 1;
  constexpr auto UpRight = 2;
  constexpr auto Right = 3;
  constexpr auto DownRight = 4;
  constexpr auto Down = 5;
  constexpr auto DownLeft = 6;
  constexpr auto Left = 7;
  constexpr auto UpLeft = 8;

  /**
   * Maps set bits for dpad values to GamepadAPI.
   *
   * Meaningful fallback for invalid combinations.
   */
  const int8_t Table[] =
  {
    Centered,   // 0000 = none
    Down,       // 0001 = down
    Right,      // 0010 = right
    DownRight,  // 0011 = down + right
    Up,         // 0100 = up
    Down,       // 0101 = down + up = invalid
    UpRight,    // 0110 = right + up
    Right,      // 0111 = right + up + down = invalid
    Left,       // 1000 = left
    DownLeft,  // 1001 = down + left
    Right,      // 1010 = right + left = invalid
    Down,       // 1011 = right + left + down = invalid
    UpLeft,     // 1100 = up + left
    Left,       // 1101 = down + up + left = invalid
    Up,         // 1110 = right + up + left = invalid
    Centered    // 1111 = all buttons = invalid
  };

  int8_t getValue(const GenesisController& controller)
  {
    byte index = byte(controller.down) | (byte(controller.right) << 1) | (byte(controller.up) << 2) | (byte(controller.left) << 3);
    return Dpad::Table[index];
  }
}


// Toggle TH and let the pad's outputs settle before we sample them.
void setSelect(bool high)
{
  digitalWrite(Pin::TH, high ? GamepadAPI::High : GamepadAPI::Low);
  delayMicroseconds(SettleTime);
}


namespace ControllerState
{
  /**
   * Reset the controller's internal state counter for this read cycle
   */
  void resetState()
  {
    setSelect(GamepadAPI::Low);
    setSelect(GamepadAPI::High);
  }


  /**
   * Read 3-Button Input State
   * 
   * TH == Low: Pins 1–4 red Up/Down/GND/GND and Pins 6/9 read A/Start.
   * TH == High, Pins 1–4 read Up/Down/Left/Right and Pins 6/9 read B/C.
   * 
   * Documentation source:
   * https://allpinouts.org/pinouts/connectors/videogame/sega-genesis-joystick-controller/
   */
  void readThreeButtonState(GenesisController& controller)
  {
    setSelect(GamepadAPI::Low);
    controller.button_a = (digitalRead(Pin::TL) == GamepadAPI::Low);
    controller.button_start = (digitalRead(Pin::TR) == GamepadAPI::Low);

    setSelect(GamepadAPI::High);
    controller.up = (digitalRead(Pin::Up) == GamepadAPI::Low);
    controller.down = (digitalRead(Pin::Down) == GamepadAPI::Low);
    controller.left = (digitalRead(Pin::Left) == GamepadAPI::Low);
    controller.right = (digitalRead(Pin::Right) == GamepadAPI::Low);
    controller.button_b = (digitalRead(Pin::TL) == GamepadAPI::Low);
    controller.button_c = (digitalRead(Pin::TR) == GamepadAPI::Low);
  }


  /**
   * Read 3-Button Input State (X/Y/X/MODE)
   * 
   * If not a six-button controller X/Y/Z/MODE will be set as
   * unpressed.
   * 
   * Documentation source:
   * https://allpinouts.org/pinouts/connectors/videogame/sega-genesis-joystick-controller/
   */
  void readSixButtonState(GenesisController& controller, bool isSixButton)
  {
    setSelect(GamepadAPI::High);
    if (!isSixButton)
    {
      controller.button_x = false;
      controller.button_y = false;
      controller.button_z = false;
      controller.button_mode = false;
      return;
    }

    controller.button_z = (digitalRead(Pin::Up) == GamepadAPI::Low);
    controller.button_y = (digitalRead(Pin::Down) == GamepadAPI::Low);
    controller.button_x = (digitalRead(Pin::Left) == GamepadAPI::Low);
    controller.button_mode = (digitalRead(Pin::Right) == GamepadAPI::Low);
  }

  /**
   * Determine if connected controller is a six button.
   * 
   * TH = LOW
   * pins 1/2 forced low together = "6-button pad present".
   */
  bool isSixButtonController(GenesisController& controller)
  {
    setSelect(GamepadAPI::Low);
    return (digitalRead(Pin::Up) == GamepadAPI::Low) && (digitalRead(Pin::Down) == GamepadAPI::Low);
  }
}


void readController(GenesisController &controller)
{
  ControllerState::resetState();

  ControllerState::readThreeButtonState(controller);
  ControllerState::readSixButtonState(controller, ControllerState::isSixButtonController(controller));

  ControllerState::resetState();
}


struct ButtonMap
{
  bool (*get)(const GenesisController &);
  uint8_t id;
};


static const ButtonMap Buttons[] =
{
  {[](const GenesisController &c) { return c.button_a; },     1},
  {[](const GenesisController &c) { return c.button_b; },     2},
  {[](const GenesisController &c) { return c.button_c; },     3},
  {[](const GenesisController &c) { return c.button_start; }, 4},
  {[](const GenesisController &c) { return c.button_x; },     5},
  {[](const GenesisController &c) { return c.button_y; },     6},
  {[](const GenesisController &c) { return c.button_z; },     7},
  {[](const GenesisController &c) { return c.button_mode; },  8},
};



void updateController(const GenesisController &controller)
{
  gamepad.releaseAll();
  gamepad.dPad(Dpad::getValue(controller));

  for (const auto &b : Buttons)
  {
    if (b.get(controller))
    {
      gamepad.press(b.id);
    }
  }

  gamepad.write();
}


void setup()
{
  pinMode(Pin::TH, OUTPUT);
  digitalWrite(Pin::TH, GamepadAPI::High);

  pinMode(Pin::Up, INPUT_PULLUP);
  pinMode(Pin::Down, INPUT_PULLUP);
  pinMode(Pin::Left, INPUT_PULLUP);
  pinMode(Pin::Right, INPUT_PULLUP);
  pinMode(Pin::TL, INPUT_PULLUP);
  pinMode(Pin::TR, INPUT_PULLUP);

  gamepad.begin();

  digitalWrite(Pin::PowerLED, GamepadAPI::High);
}


void loop()
{
  const auto now = millis();

  GenesisController controller;
  readController(controller);
  updateController(controller);

  const auto delta = millis() - now;
  if (delta < PollInterval)
  {
    delay(PollInterval - delta);
  }
}
