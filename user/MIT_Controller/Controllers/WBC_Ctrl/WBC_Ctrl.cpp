#include "WBC_Ctrl.hpp"
#include <Utilities/Utilities_print.h>


template<typename T>
WBC_Ctrl<T>::WBC_Ctrl(FloatingBaseModel<T> model):
  _full_config(cheetah::num_act_joint + 7),
  _tau_ff(cheetah::num_act_joint),
  _des_jpos(cheetah::num_act_joint),
  _des_jvel(cheetah::num_act_joint),
  _des_jacc(cheetah::num_act_joint),
  _wbcLCM(getLcmUrl(255))
{
  _iter = 0;
  _full_config.setZero();

  _model = model;
  _kin_wbc = new KinWBC<T>(cheetah::dim_config);
  _wbic = new WBIC<T>(cheetah::dim_config, &(_contact_list), &(_task_list));

  _wbic_data = new WBIC_ExtraData<T>();
  _wbic_data->_W_floating = DVec<T>::Constant(6, 0.1);
  _wbic_data->_W_rf = DVec<T>::Constant(12, 1.);

  _Kp_joint.resize(cheetah::num_leg_joint, 5.);
  _Kd_joint.resize(cheetah::num_leg_joint, 1.5);

  _state.q = DVec<T>::Zero(cheetah::num_act_joint);
  _state.qd = DVec<T>::Zero(cheetah::num_act_joint);
}

template<typename T>
WBC_Ctrl<T>::~WBC_Ctrl(){
  delete _kin_wbc;
  delete _wbic;
  delete _wbic_data;

  typename std::vector<Task<T> *>::iterator iter = _task_list.begin();
  while (iter < _task_list.end()) {
    delete (*iter);
    ++iter;
  }
  _task_list.clear();

  typename std::vector<ContactSpec<T> *>::iterator iter2 = _contact_list.begin();
  while (iter2 < _contact_list.end()) {
    delete (*iter2);
    ++iter2;
  }
  _contact_list.clear();
}

template <typename T>
void WBC_Ctrl<T>::_ComputeWBC() {
  _kin_wbc->FindConfiguration(_full_config, _task_list, _contact_list,
                              _des_jpos, _des_jvel, _des_jacc);

  // WBIC
  _wbic->UpdateSetting(_A, _Ainv, _coriolis, _grav);
  _wbic->MakeTorque(_tau_ff, _wbic_data);
}

template<typename T>
void WBC_Ctrl<T>::run(void* input, ControlFSMData<T> & data){
  ++_iter;

  // Update Model
  _UpdateModel(data._stateEstimator->getResult(), data._legController->datas);

  // Task & Contact Update
  _ContactTaskUpdate(input, data);

  // WBC Computation
  _ComputeWBC();

  // Update Leg Command
  _UpdateLegCMD(data._legController->commands);
}



template<typename T>
void WBC_Ctrl<T>::_UpdateLegCMD(LegControllerCommand<T> * cmd){
  for (size_t leg(0); leg < cheetah::num_leg; ++leg) {
    cmd[leg].zero();
    for (size_t jidx(0); jidx < cheetah::num_leg_joint; ++jidx) {
      cmd[leg].tauFeedForward[jidx] = _tau_ff[cheetah::num_leg_joint * leg + jidx];
      cmd[leg].qDes[jidx] = _des_jpos[cheetah::num_leg_joint * leg + jidx];
      cmd[leg].qdDes[jidx] = _des_jvel[cheetah::num_leg_joint * leg + jidx];

      cmd[leg].kpJoint(jidx, jidx) = _Kp_joint[jidx];
      cmd[leg].kdJoint(jidx, jidx) = _Kd_joint[jidx];
    }
  }
}

template<typename T>
void WBC_Ctrl<T>::_UpdateModel(const StateEstimate<T> & state_est, 
    const LegControllerData<T> * leg_data){

  _state.bodyOrientation = state_est.orientation;
  _state.bodyPosition = state_est.position;
  for(size_t i(0); i<3; ++i){
    _state.bodyVelocity[i] = state_est.omegaBody[i];
    _state.bodyVelocity[i+3] = state_est.vBody[i];

    for(size_t leg(0); leg<4; ++leg){
      _state.q[3*leg + i] = leg_data[leg].q[i];
      _state.qd[3*leg + i] = leg_data[leg].qd[i];

      _full_config[3*leg + i + 6] = _state.q[3*leg + i];
    }
  }
  _model.setState(_state);

  _model.contactJacobians();
  _model.massMatrix();
  _model.generalizedGravityForce();
  _model.generalizedCoriolisForce();

  _A = _model.getMassMatrix();
  _grav = _model.getGravityForce();
  _coriolis = _model.getCoriolisForce();
  _Ainv = _A.inverse();
}


template class WBC_Ctrl<float>;
template class WBC_Ctrl<double>;
