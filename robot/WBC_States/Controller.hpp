#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <cppTypes.h>
#include <Dynamics/FloatingBaseModel.h>
#include <WBC/Task.hpp>
#include <WBC/ContactSpec.hpp>

#define Ctrl Controller<T>

namespace Weight{
    constexpr float tan_big = 5.;
    constexpr float tan_small = 1.;
    constexpr float nor_big = 0.5;
    constexpr float nor_small = 0.01;
    constexpr float foot_big = 100.;
    constexpr float foot_small = 0.01;
    constexpr float qddot_relax = 30.;
    constexpr float qddot_relax_virtual = 0.001;
    //constexpr float qddot_relax_virtual = 100.0;
}


template <typename T>
class Controller{
public:
  Controller(const FloatingBaseModel<T>* robot):
      _robot_sys(robot),_state_machine_time(0.){}
  virtual ~Controller(){}

  virtual void OneStep(void* command) = 0;
  virtual void FirstVisit() = 0;
  virtual void LastVisit() = 0;
  virtual bool EndOfPhase() = 0;
  virtual void CtrlInitialization(const std::string & setting_file_name) = 0;
  virtual void SetTestParameter(const std::string & test_file) = 0;

  DMat<T> _A;
  DMat<T> _Ainv;
  DVec<T> _grav;
  DVec<T> _coriolis;

protected:
  std::vector<Task<T>*> _task_list;
  std::vector<ContactSpec<T>*> _contact_list;

  void _DynConsistent_Inverse(const DMat<T> & J, DMat<T> & Jinv){
      DMat<T> Jtmp(J * _Ainv * J.transpose());
      Jinv = _Ainv * J.transpose() * Jtmp.inverse();
  }

  void _PreProcessing_Command(){
      _A = _robot_sys->getMassMatrix();
      _grav = _robot_sys->getGravityForce();
      _coriolis = _robot_sys->getCoriolisForce();
      _Ainv = _A.inverse();
  }

  void _PostProcessing_Command(){
      //for(size_t i(0); i<_task_list.size(); ++i){ _task_list[i]->UnsetTask(); }
      //for(size_t i(0); i<_contact_list.size(); ++i){ _contact_list[i]->UnsetContact(); }
  }

  const FloatingBaseModel<T>* _robot_sys;

  T _state_machine_time;
};

#endif
