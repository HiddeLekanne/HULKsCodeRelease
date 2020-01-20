#pragma once

#include "Framework/DataType.hpp"
#include "Modules/NaoProvider.h"
#include <array>

class JointCalibrationData : public DataType<JointCalibrationData>
{
public:
  /// the name of this DataType
  DataTypeName name = "JointCalibrationData";
  /// the offset of all joints in rad (is added to the final joint values and subtracted from the
  /// measurements)
  std::array<float, JOINTS::JOINTS_MAX> calibrationOffsets;

  void reset() override
  {
    // This is empty on purpose since the calibration provider does not rewrite the offsets every
    // cycle
  }

  void toValue(Uni::Value& value) const override
  {
    value = Uni::Value(Uni::ValueType::OBJECT);
    value["calibrationOffsets"] << calibrationOffsets;
  }

  void fromValue(const Uni::Value& value) override
  {
    value["calibrationOffsets"] >> calibrationOffsets;
  }
};
