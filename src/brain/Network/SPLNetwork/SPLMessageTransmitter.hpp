#pragma once

#include "Framework/Module.hpp"
#include "Tools/Time.hpp"

#include "Definitions/DevilSmashStandardMessageTest.hpp"

#include "Data/BallSearchMap.hpp"
#include "Data/BallSearchPosition.hpp"
#include "Data/BallState.hpp"
#include "Data/BodyPose.hpp"
#include "Data/CycleInfo.hpp"
#include "Data/GameControllerState.hpp"
#include "Data/JointSensorData.hpp"
#include "Data/KeeperAction.hpp"
#include "Data/MotionRequest.hpp"
#include "Data/NTPData.hpp"
#include "Data/NetworkServiceData.hpp"
#include "Data/ObstacleData.hpp"
#include "Data/PlayerConfiguration.hpp"
#include "Data/PlayingRoles.hpp"
#include "Data/RobotPosition.hpp"
#include "Data/SPLNetworkData.hpp"
#include "Data/StrikerAction.hpp"
#include "Data/TeamBallModel.hpp"
#include "Data/TimeToReachBall.hpp"
#include "Data/WhistleData.hpp"

class Brain;

class SPLMessageTransmitter : public Module<SPLMessageTransmitter, Brain>
{
public:
  /// the name of this module
  ModuleName name = "SPLMessageTransmitter";
  /**
   * @brief SPLMessageTransmitter initializes members
   * @param manager reference to brain
   */
  explicit SPLMessageTransmitter(const ModuleManagerInterface& manager);
  /**
   * @brief cycle transmits an SPL message if enough time has elapsed since the last message
   */
  void cycle() override;

private:
  /// How many messages should be sent per second
  const Parameter<float> msgPerSecond_;
  /// Whether we want to transmit SPL standard messages via ethernet.
  const Parameter<bool> transmitViaEthernet_;
  /// whether sonar obstacles should be sent in the B-HULKs message
  const Parameter<bool> sendSonarObstacles_;
  /// whether the transmitter should fake the member flag (aka saying that we are a non HULK robot)
  const Parameter<bool> fakeMemberFlag_;
  /// player and team number are needed for identification
  const Dependency<PlayerConfiguration> playerConfiguration_;
  /// Network service data to determine if there is any wifi connected
  const Dependency<NetworkServiceData> networkServiceData_;
  /// ball state is used for obvious reasons
  const Dependency<BallState> ballState_;
  /// robot position is used for obvious reasons
  const Dependency<RobotPosition> robotPosition_;
  /// the pose of the robot body
  const Dependency<BodyPose> bodyPose_;
  /// needed for message sending
  const Dependency<SPLNetworkData> splNetworkData_;
  /// the playing role and assignment for other players
  const Dependency<PlayingRoles> playingRoles_;
  /// the motion request of the player
  const Dependency<MotionRequest> motionRequest_;
  /// the received NTP requests from this cycle
  const Dependency<NTPData> ntpData_;
  /// the action of the striker
  const Dependency<StrikerAction> strikerAction_;
  /// the action of the keeper
  const Dependency<KeeperAction> keeperAction_;
  /// the data about the detection of the whistle
  const Dependency<WhistleData> whistleData_;
  /// tremendously useless comment
  const Dependency<TimeToReachBall> timeToReachBall_;
  /// the cycle info
  const Dependency<CycleInfo> cycleInfo_;
  /// the game controller state
  const Dependency<GameControllerState> gameControllerState_;
  /// the own obstacle data
  const Dependency<ObstacleData> obstacleData_;
  /// the joint sensor data
  const Dependency<JointSensorData> jointSensorData_;
  /// the team ball
  const Dependency<TeamBallModel> teamBallModel_;
  /// the ball search map
  const Dependency<BallSearchMap> ballSearchMap_;
  /// the ball search position
  const Dependency<BallSearchPosition> ballSearchPosition_;
  /// the last time when a message was sent
  TimePoint lastTime_;
  /// the last time when a NTP message has been requested
  TimePoint lastNTPRequest_;
  /// a list of all NTP requests that have not been answered yet (because no SPL message was allowed
  /// to be sent)
  std::vector<NTPData::NTPRequest> bufferedNTPRequests_;
};
