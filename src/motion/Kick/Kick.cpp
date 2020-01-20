#include "Modules/NaoProvider.h"
#include "Modules/Poses.h"
#include "Tools/Kinematics/Com.h"
#include "Tools/Kinematics/ForwardKinematics.h"
#include "Tools/Kinematics/InverseKinematics.h"
#include "Tools/Math/Angle.hpp"

#include "Kick.hpp"


Kick::Kick(const ModuleManagerInterface& manager)
  : Module(manager)
  , cycleInfo_(*this)
  , imuSensorData_(*this)
  , jointSensorData_(*this)
  , kickConfigurationData_(*this)
  , motionActivation_(*this)
  , motionRequest_(*this)
  , kickOutput_(*this)
  , leftKicking_(true)
  , torsoOffsetLeft_(*this, "torsoOffsetLeft", [] {})
  , torsoOffsetRight_(*this, "torsoOffsetRight", [] {})
  , currentInterpolatorID_(interpolators_.size())
  , gyroLowPassRatio_(*this, "gyroLowPassRatio", [] {})
  , gyroForwardBalanceFactor_(*this, "gyroForwardBalanceFactor", [] {})
  , gyroSidewaysBalanceFactor_(*this, "gyroSidewaysBalanceFactor", [] {})
  , filteredGyro_(Vector2f::Zero())
{
}

void Kick::cycle()
{
  // update gyroscope filter
  filteredGyro_.x() = gyroLowPassRatio_() * filteredGyro_.x() +
                      (1.f - gyroLowPassRatio_()) * imuSensorData_->gyroscope.x();
  filteredGyro_.y() = gyroLowPassRatio_() * filteredGyro_.y() +
                      (1.f - gyroLowPassRatio_()) * imuSensorData_->gyroscope.y();

  // check if a kick is requested
  const bool incomingKickRequest =
      motionActivation_->activations[static_cast<unsigned int>(MotionRequest::BodyMotion::KICK)] ==
          1 &&
      motionRequest_->bodyMotion == MotionRequest::BodyMotion::KICK;
  if (currentInterpolatorID_ == interpolators_.size() && incomingKickRequest)
  {
    // get kick kick configuration based on requested kick type
    const KickConfiguration& kickConfiguration =
        kickConfigurationData_->kicks[static_cast<unsigned int>(motionRequest_->kickData.kickType)];
    // check whether left or right foot is to be used
    leftKicking_ = motionRequest_->kickData.ballSource.y() > 0;
    // select appropriate torso offset
    const Vector3f torsoOffset = leftKicking_ ? torsoOffsetLeft_() : torsoOffsetRight_();
    // reset interpolators
    resetInterpolators(kickConfiguration, torsoOffset);
    // initialize kick
    currentInterpolatorID_ = 0;
  }

  // check whether kick if active
  if (currentInterpolatorID_ < interpolators_.size())
  {
    // do not move this check unless you want a segmentation fault
    if (interpolators_[currentInterpolatorID_]->finished())
    {
      // advance kick phase
      currentInterpolatorID_++;
    }
  }

  // check whether kick if active
  if (currentInterpolatorID_ < interpolators_.size())
  {
    // convert seconds to milliseconds to get time step
    const float timeStep = cycleInfo_->cycleTime * 1000;
    // get output angles from current interpolator
    std::vector<float> outputAngles = interpolators_[currentInterpolatorID_]->step(timeStep);
    // apply gyroscope feedback
    gyroFeedback(outputAngles);
    kickOutput_->angles = outputAngles;
    kickOutput_->stiffnesses = std::vector<float>(JOINTS::JOINTS_MAX, 0.7f);
    kickOutput_->safeExit = false;

    // mirror output angles if right foot is used
    if (!leftKicking_)
    {
      kickOutput_->mirrorAngles();
    }
  }
  else
  {
    // default kick output
    kickOutput_->angles = Poses::getPose(Poses::READY);
    kickOutput_->stiffnesses = std::vector<float>(JOINTS::JOINTS_MAX, 0.7f);
    kickOutput_->safeExit = true;
  }
}

void Kick::resetInterpolators(const KickConfiguration& kickConfiguration, const Vector3f& torsoOffset)
{
  /*
   * wait before start
   */
  const std::vector<float> anglesAtKickRequest = jointSensorData_->getBodyAngles();
  const std::vector<float> readyPoseAngles = Poses::getPose(Poses::READY);
  waitBeforeStartInterpolator_.reset(anglesAtKickRequest, readyPoseAngles,
                                     kickConfiguration.waitBeforeStartDuration);

  /*
   * weight shift
   */
  const Vector3f weightShiftCom = kickConfiguration.weightShiftCom + torsoOffset;
  std::vector<float> weightShiftAngles(JOINTS::JOINTS_MAX);
  computeWeightShiftAnglesFromReferenceCom(readyPoseAngles, weightShiftCom, weightShiftAngles);
  weightShiftAngles[JOINTS::L_SHOULDER_ROLL] = kickConfiguration.shoulderRoll;
  weightShiftAngles[JOINTS::R_SHOULDER_ROLL] = -kickConfiguration.shoulderRoll;
  weightShiftInterpolator_.reset(readyPoseAngles, weightShiftAngles,
                                 kickConfiguration.weightShiftDuration);

  /*
   * lift foot
   */
  const float yawLeft2right = kickConfiguration.yawLeft2right;
  const KinematicMatrix liftFootPose = KinematicMatrix(AngleAxisf(yawLeft2right, Vector3f::UnitZ()),
                                                       kickConfiguration.liftFootPosition);
  std::vector<float> liftFootAngles(JOINTS::JOINTS_MAX);
  computeLegAnglesFromFootPose(weightShiftAngles, liftFootPose, liftFootAngles);
  liftFootAngles[JOINTS::L_SHOULDER_PITCH] -= kickConfiguration.shoulderPitchAdjustment;
  liftFootAngles[JOINTS::R_SHOULDER_PITCH] += kickConfiguration.shoulderPitchAdjustment;
  liftFootAngles[JOINTS::L_ANKLE_ROLL] = kickConfiguration.ankleRoll;
  liftFootInterpolator_.reset(weightShiftAngles, liftFootAngles, kickConfiguration.liftFootDuration);

  /*
   * swing foot
   */
  const KinematicMatrix swingFootPose = KinematicMatrix(
      AngleAxisf(yawLeft2right, Vector3f::UnitZ()), kickConfiguration.swingFootPosition);
  std::vector<float> swingFootAngles(JOINTS::JOINTS_MAX);
  computeLegAnglesFromFootPose(liftFootAngles, swingFootPose, swingFootAngles);
  swingFootAngles[JOINTS::L_SHOULDER_PITCH] += kickConfiguration.shoulderPitchAdjustment;
  swingFootAngles[JOINTS::R_SHOULDER_PITCH] -= kickConfiguration.shoulderPitchAdjustment;
  swingFootAngles[JOINTS::L_ANKLE_PITCH] += kickConfiguration.anklePitch;
  swingFootAngles[JOINTS::L_ANKLE_ROLL] = kickConfiguration.ankleRoll;
  swingFootInterpolator_.reset(liftFootAngles, swingFootAngles, kickConfiguration.swingFootDuration);

  /*
   * kick ball
   */
  const KinematicMatrix kickBallPose = KinematicMatrix(AngleAxisf(yawLeft2right, Vector3f::UnitZ()),
                                                       kickConfiguration.kickBallPosition);
  std::vector<float> kickBallAngles(JOINTS::JOINTS_MAX);
  computeLegAnglesFromFootPose(swingFootAngles, kickBallPose, kickBallAngles);
  kickBallAngles[JOINTS::L_SHOULDER_PITCH] += kickConfiguration.shoulderPitchAdjustment;
  kickBallAngles[JOINTS::R_SHOULDER_PITCH] -= kickConfiguration.shoulderPitchAdjustment;
  kickBallAngles[JOINTS::L_ANKLE_ROLL] = kickConfiguration.ankleRoll;
  kickBallInterpolator_.reset(swingFootAngles, kickBallAngles, kickConfiguration.kickBallDuration);

  /*
   * pause
   */
  pauseInterpolator_.reset(kickBallAngles, kickBallAngles, kickConfiguration.pauseDuration);

  /*
   * retract foot
   */
  const KinematicMatrix retractFootPose = KinematicMatrix(
      AngleAxisf(yawLeft2right, Vector3f::UnitZ()), kickConfiguration.retractFootPosition);
  std::vector<float> retractFootAngles(JOINTS::JOINTS_MAX);
  computeLegAnglesFromFootPose(kickBallAngles, retractFootPose, retractFootAngles);
  retractFootAngles[JOINTS::L_SHOULDER_PITCH] -= kickConfiguration.shoulderPitchAdjustment;
  retractFootAngles[JOINTS::R_SHOULDER_PITCH] += kickConfiguration.shoulderPitchAdjustment;
  retractFootAngles[JOINTS::L_ANKLE_ROLL] = kickConfiguration.ankleRoll;
  retractFootInterpolator_.reset(kickBallAngles, retractFootAngles,
                                 kickConfiguration.retractFootDuration);

  /*
   * extend foot and center torso
   */
  extendFootAndCenterTorsoInterpolator_.reset(retractFootAngles, readyPoseAngles,
                                              kickConfiguration.extendFootAndCenterTorsoDuration);

  /*
   * wait before exit
   */
  waitBeforeExitInterpolator_.reset(readyPoseAngles, readyPoseAngles,
                                    kickConfiguration.waitBeforeExitDuration);
}

void Kick::computeWeightShiftAnglesFromReferenceCom(const std::vector<float>& currentAngles,
                                                    const Vector3f& weightShiftCom,
                                                    std::vector<float>& weightShiftAngles) const
{
  weightShiftAngles = currentAngles;
  // iteratively move the torso to achieve the desired CoM
  for (unsigned int i = 0; i < 5; i++)
  {
    std::vector<float> leftLegAngles(JOINTS_L_LEG::L_LEG_MAX);
    std::vector<float> rightLegAngles(JOINTS_R_LEG::R_LEG_MAX);
    separateAngles(leftLegAngles, rightLegAngles, weightShiftAngles);

    KinematicMatrix com2torso = Com::getCom(weightShiftAngles);
    const KinematicMatrix right2torso = ForwardKinematics::getRFoot(rightLegAngles);
    const KinematicMatrix com2right = right2torso.invert() * com2torso;
    const KinematicMatrix left2torso = ForwardKinematics::getLFoot(leftLegAngles);
    const KinematicMatrix com2left = left2torso.invert() * com2torso;

    const Vector3f comError = com2right.posV - weightShiftCom;

    com2torso.posV += comError;

    leftLegAngles = InverseKinematics::getLLegAngles(com2torso * com2left.invert());
    rightLegAngles = InverseKinematics::getFixedRLegAngles(
        com2torso * com2right.invert(), leftLegAngles[JOINTS_L_LEG::L_HIP_YAW_PITCH]);
    combineAngles(weightShiftAngles, currentAngles, leftLegAngles, rightLegAngles);
  }
}

void Kick::computeLegAnglesFromFootPose(const std::vector<float>& currentAngles,
                                        const KinematicMatrix& nextLeft2right,
                                        std::vector<float>& nextAngles) const
{
  std::vector<float> leftLegAngles(JOINTS_L_LEG::L_LEG_MAX);
  std::vector<float> rightLegAngles(JOINTS_R_LEG::R_LEG_MAX);
  separateAngles(leftLegAngles, rightLegAngles, currentAngles);

  // compute left and right foot pose relative to torso
  const KinematicMatrix right2torso = ForwardKinematics::getRFoot(rightLegAngles);
  const KinematicMatrix left2torso = right2torso * nextLeft2right;

  // compute left and right leg angles
  leftLegAngles = InverseKinematics::getLLegAngles(left2torso);
  rightLegAngles = InverseKinematics::getFixedRLegAngles(
      right2torso, leftLegAngles[JOINTS_L_LEG::L_HIP_YAW_PITCH]);

  combineAngles(nextAngles, currentAngles, leftLegAngles, rightLegAngles);
}

void Kick::separateAngles(std::vector<float>& left, std::vector<float>& right,
                          const std::vector<float>& body) const
{
  left.resize(JOINTS_L_LEG::L_LEG_MAX);
  right.resize(JOINTS_R_LEG::R_LEG_MAX);
  for (unsigned int i = 0; i < JOINTS_L_LEG::L_LEG_MAX; i++)
  {
    left[i] = body[JOINTS::L_HIP_YAW_PITCH + i];
    right[i] = body[JOINTS::R_HIP_YAW_PITCH + i];
  }
}

void Kick::combineAngles(std::vector<float>& result, const std::vector<float>& body,
                         const std::vector<float>& left, const std::vector<float>& right) const
{
  result = body;
  for (unsigned int i = 0; i < JOINTS_L_LEG::L_LEG_MAX; i++)
  {
    result[JOINTS::L_HIP_YAW_PITCH + i] = left[i];
    result[JOINTS::R_HIP_YAW_PITCH + i] = right[i];
  }
}

void Kick::gyroFeedback(std::vector<float>& outputAngles) const
{
  // add filtered gyroscope x and y values multiplied by gain to ankle roll and pitch, respectively
  outputAngles[JOINTS::R_ANKLE_ROLL] +=
      (leftKicking_ ? 1 : -1) * gyroSidewaysBalanceFactor_() * filteredGyro_.x();
  outputAngles[JOINTS::R_ANKLE_PITCH] += gyroForwardBalanceFactor_() * filteredGyro_.y();
}
