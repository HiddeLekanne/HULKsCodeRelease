#pragma once

#include <Framework/DataType.hpp>

#include "MotionRequest.hpp"

class MotionState : public DataType<MotionState>
{
public:
  /// the name of this DataType
  DataTypeName name = "MotionState";
  /// the motion that the body (legs + potentially arms + potentially head) executes
  MotionRequest::BodyMotion bodyMotion;
  /// the motion that the left arm executes
  MotionRequest::ArmMotion leftArmMotion;
  /// the motion that the right arm executes
  MotionRequest::ArmMotion rightArmMotion;
  /// the motion that the head executes
  MotionRequest::HeadMotion headMotion;
  /// the angles (this is the motion output that is send to the robot interface)
  std::vector<float> angles;

  /**
   * @brief reset sets the robot dead
   */
  void reset() override
  {
    bodyMotion = MotionRequest::BodyMotion::DEAD;
    leftArmMotion = MotionRequest::ArmMotion::BODY;
    rightArmMotion = MotionRequest::ArmMotion::BODY;
    headMotion = MotionRequest::HeadMotion::BODY;
    angles.clear();
  }

  void toValue(Uni::Value& value) const override
  {
    value = Uni::Value(Uni::ValueType::OBJECT);
    value["bodyMotion"] << static_cast<int>(bodyMotion);
    value["leftArmMotion"] << static_cast<int>(leftArmMotion);
    value["rightArmMotion"] << static_cast<int>(rightArmMotion);
    value["headMotion"] << static_cast<int>(headMotion);
    value["angles"] << angles;
  }

  void fromValue(const Uni::Value& value) override
  {
    int readValue = 0;
    value["bodyMotion"] >> readValue;
    bodyMotion = static_cast<MotionRequest::BodyMotion>(readValue);
    value["leftArmMotion"] >> readValue;
    leftArmMotion = static_cast<MotionRequest::ArmMotion>(readValue);
    value["rightArmMotion"] >> readValue;
    rightArmMotion = static_cast<MotionRequest::ArmMotion>(readValue);
    value["headMotion"] >> readValue;
    headMotion = static_cast<MotionRequest::HeadMotion>(readValue);
    value["angles"] >> angles;
  }
};
