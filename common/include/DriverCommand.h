/*! @file DriverCommand.h
 *  @brief The DriverCommand type containing joystick information
 */

#ifndef PROJECT_DRIVERCOMMAND_H
#define PROJECT_DRIVERCOMMAND_H

#include "cppTypes.h"
#include "utilities.h"

struct DriverCommand {
  DriverCommand() {
    zero();
  }

  bool leftBumper, rightBumper, leftTriggerButton, rightTriggerButton,
  back, start, a, b, x, y, leftStickButton, rightStickButton, logitechButton;

  Vec2<float> leftStickAnalog, rightStickAnalog;
  float leftTriggerAnalog, rightTriggerAnalog;

  void zero() {
    leftBumper = false;
    rightBumper = false;
    leftTriggerButton = false;
    rightTriggerButton = false;
    back = false;
    start = false;
    a = false;
    b = false;
    x = false;
    y = false;
    leftStickButton = false;
    rightStickButton = false;

    leftTriggerAnalog = 0;
    rightTriggerAnalog = 0;
    leftStickAnalog = Vec2<float>::Zero();
    rightStickAnalog = Vec2<float>::Zero();
  }

  std::string toString() {
    std::string result =
                    "leftBumper: " + boolToString(leftBumper) + "\n" +
                    "rightBumper: " + boolToString(rightBumper) + "\n" +
                    "leftTriggerButton: " + boolToString(leftTriggerButton) + "\n" +
                    "rightTriggerButton: " + boolToString(rightTriggerButton) + "\n" +
                    "back: " + boolToString(back) + "\n" +
                    "start: " + boolToString(start) + "\n" +
                    "a: " + boolToString(a) + "\n" +
                    "b: " + boolToString(b) + "\n" +
                    "x: " + boolToString(x) + "\n" +
                    "y: " + boolToString(y) + "\n" +
                    "leftStickButton: " + boolToString(leftStickButton) + "\n" +
                    "rightStickButton: " + boolToString(rightStickButton) + "\n" +
                    "leftTriggerAnalog: " + std::to_string(leftTriggerAnalog) + "\n" +
                    "rightTriggerAnalog: " + std::to_string(rightTriggerAnalog) + "\n" +
                    "leftStickAnalog: " + eigenToString(leftStickAnalog) + "\n" +
                    "rightStickAnalog: " + eigenToString(rightStickAnalog) + "\n";
    return result;
  }
};

#endif //PROJECT_DRIVERCOMMAND_H
