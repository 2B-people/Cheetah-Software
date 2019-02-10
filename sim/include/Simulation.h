#ifndef PROJECT_SIMULATION_H
#define PROJECT_SIMULATION_H

#include "Dynamics/Quadruped.h"
#include "Graphics3D.h"
#include "Dynamics/MiniCheetah.h"
#include "Dynamics/Cheetah3.h"
#include "Utilities/Timer.h"
#include "SimUtilities/SpineBoard.h"
#include "SimUtilities/ti_boardcontrol.h"
#include "Utilities/SharedMemory.h"
#include "SimUtilities/SimulatorMessage.h"
#include "ControlParameters/SimulatorParameters.h"
#include "ControlParameters/RobotParameters.h"
#include "SimUtilities/ImuSimulator.h"
#include "ControlParameters/ControlParameterInterface.h"

#include <vector>
#include <mutex>
#include <queue>
#include <utility>

#include <lcm/lcm-cpp.hpp>
#include "simulator_lcmt.hpp"

#define SIM_LCM_NAME "simulator_state"


/*!
 * Top-level control of a simulation.
 * A simulation includes 1 robot and 1 controller
 * It does not include the graphics window: this must be set with the setWindow method
 */
class Simulation {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  explicit Simulation(RobotType robot, Graphics3D* window, SimulatorControlParameters& params);

  /*!
   * Explicitly set the state of the robot
   */
  void setRobotState(FBModelState<double>& state) {
    _simulator->setState(state);
  }


  void step(double dt, double dtLowLevelControl, double dtHighLevelControl);

  void addCollisionPlane(double mu, double resti, double height, bool addToWindow = true);
  void addCollisionBox(
          double mu, double resti, 
          double depth, double width, double height, 
          const Vec3<double> & pos, const Mat3<double> & ori,
          bool addToWindow = true);

  void lowLevelControl();
  void highLevelControl();

  /*!
   * Updates the graphics from the connected window
   */
  void updateGraphics() {
    _window->_drawList.updateRobotFromModel(*_simulator, _robotID);
    _window->_drawList.updateAdditionalInfo(*_simulator);
    _window->update();
  }

  void freeRun(double dt, double dtLowLevelControl, double dtHighLevelControl, bool graphics = true);
  void runAtSpeed(bool graphics = true);
  void sendControlParameter(const std::string& name, ControlParameterValue value, ControlParameterValueKind kind);

  void resetSimTime() {
    _currentSimTime = 0.;
    _timeOfNextLowLevelControl = 0.;
    _timeOfNextHighLevelControl = 0.;
  }


  ~Simulation() {
      delete _simulator;
      delete _imuSimulator;
      delete _lcm;
  }

  const FBModelState<double>& getRobotState() {
    return _simulator->getState();
  }

  void stop() {
    _running = false; // kill simulation loop
    _wantStop = true; // if we're still trying to connect, this will kill us

    if(_connected) {
      _sharedMemory().simToRobot.mode = SimulatorMode::EXIT;
      _sharedMemory().simulatorIsDone();
    }
  }

  SimulatorControlParameters& getSimParams() {
    return _simParams;
  }

  RobotControlParameters& getRobotParams() {
    return _robotParams;
  }

  bool isRobotConnected() {
    return _connected;
  }

  void firstRun();
  void buildLcmMessage();

private:
  std::mutex _robotMutex;
  SharedMemoryObject<SimulatorSyncronizedMessage> _sharedMemory;
  ImuSimulator<double>* _imuSimulator = nullptr;
  SimulatorControlParameters& _simParams;
  RobotControlParameters _robotParams;
  size_t _robotID;
  Graphics3D *_window = nullptr;
  Quadruped<double> _quadruped;
  FloatingBaseModel<double> _model;
  DVec<double> _tau;
  DynamicsSimulator<double>* _simulator = nullptr;
  std::vector<ActuatorModel<double>> _actuatorModels;
  SpiCommand _spiCommand;
  SpiData    _spiData;
  SpineBoard _spineBoards[4];
  TI_BoardControl _tiBoards[4];
  RobotType  _robot;
  lcm::LCM* _lcm = nullptr;
  bool _running = false;
  bool _connected = false;
  bool _wantStop = false;
  double _desiredSimSpeed = 1.;
  double _currentSimTime = 0.;
  double _timeOfNextLowLevelControl = 0.;
  double _timeOfNextHighLevelControl = 0.;
  s64 _highLevelIterations = 0;
  simulator_lcmt _simLCM;
};

#endif //PROJECT_SIMULATION_H
