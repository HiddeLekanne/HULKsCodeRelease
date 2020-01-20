#pragma once

#include "Behavior/Units.hpp"
#include "Tools/SelectWalkMode.hpp"

ActionCommand defender(const DataSet& d)
{
  if (!d.defenderAction.valid)
  {
    Log(LogLevel::WARNING) << "invalid defender action";
    return ActionCommand::stand().combineHead(activeVision(d, VisionMode::LOOK_AROUND));
  }
  switch (d.defenderAction.type)
  {
    case DefenderAction::Type::GENUFLECT:
    {
      return ActionCommand::jump(SQUAT);
    }
    case DefenderAction::Type::DEFEND:
    {
      if (d.defendingPosition.valid)
      {
        const Vector2f relBallPosition = d.robotPosition.fieldToRobot(d.teamBallModel.position);
        const float relBallAngle = std::atan2(relBallPosition.y(), relBallPosition.x());
        const Pose relPlayingPose =
            Pose(d.robotPosition.fieldToRobot(d.defendingPosition.position), relBallAngle);

        // select walk mode
        const float distanceThreshold = 1.5f;
        const float angleThreshold = 30 * TO_RAD;
        const WalkMode walkMode = SelectWalkMode::pathOrPathWithOrientation(
            relPlayingPose, distanceThreshold, angleThreshold);

        return walkToPose(d, relPlayingPose, false, walkMode, Velocity(), 5)
            .combineHead(activeVision(d, VisionMode::LOOK_AROUND_BALL));
      }
      else
      {
        Log(LogLevel::WARNING) << "invalid defending position";
        return ActionCommand::stand().combineHead(activeVision(d, VisionMode::LOOK_AROUND));
      }
    }
    default:
    {
      Log(LogLevel::WARNING) << "Invalid defender action";
      return ActionCommand::stand().combineHead(activeVision(d, VisionMode::LOOK_AROUND));
    }
  }
}
