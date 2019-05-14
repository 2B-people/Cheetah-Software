#include "WBLC_TwoContactTransCtrl.hpp"

#include <WBC_States/StateProvider.hpp>
#include <WBC_States/Cheetah_DynaCtrl_Definition.h>
#include <WBC_States/common/ContactSet/SingleContact.hpp>
#include <WBC_States/common/TaskSet/LinkPosTask.hpp>
#include <WBC_States/common/TaskSet/BodyOriTask.hpp>
#include <WBC_States/common/TaskSet/BodyPosTask.hpp>

#include <WBC/WBLC/KinWBC.hpp>
#include <WBC/WBLC/WBLC.hpp>
#include <WBC_States/WBLCTrot/WBLCTrotTest.hpp>

template <typename T>
WBLC_TwoContactTransCtrl<T>::WBLC_TwoContactTransCtrl(
    WBLCTrotTest<T> * test, const FloatingBaseModel<T>* robot, 
    size_t cp1, size_t cp2, int transit_dir):
  Controller<T>(robot),
  _trot_test(test),
  _cp1(cp1), _cp2(cp2), _transit_dir(transit_dir),
  Kp_(cheetah::num_act_joint),
  Kd_(cheetah::num_act_joint),
  des_jpos_(cheetah::num_act_joint),
  des_jvel_(cheetah::num_act_joint),
  des_jacc_(cheetah::num_act_joint),
  b_set_height_target_(false),
  end_time_(100.),
  _dim_contact(0),
  ctrl_start_time_(0.)
{
  _body_pos_task = new BodyPosTask<T>(Ctrl::_robot_sys);
  _body_ori_task = new BodyOriTask<T>(Ctrl::_robot_sys);

  Ctrl::_task_list.push_back(_body_ori_task);
  Ctrl::_task_list.push_back(_body_pos_task);

  // Pushback sequence is important !!
  fr_contact_ = new SingleContact<T>(Ctrl::_robot_sys, linkID::FR);
  fl_contact_ = new SingleContact<T>(Ctrl::_robot_sys, linkID::FL);
  hr_contact_ = new SingleContact<T>(Ctrl::_robot_sys, linkID::HR);
  hl_contact_ = new SingleContact<T>(Ctrl::_robot_sys, linkID::HL);

  Ctrl::_contact_list.push_back(fr_contact_);
  Ctrl::_contact_list.push_back(fl_contact_);
  Ctrl::_contact_list.push_back(hr_contact_);
  Ctrl::_contact_list.push_back(hl_contact_);

  DVec<T> Fr_des(3); Fr_des.setZero();
  Fr_des[2] = 9.*9.81/4.;
  typename std::vector<ContactSpec<T> *>::iterator iter = Ctrl::_contact_list.begin();
  while(iter < Ctrl::_contact_list.end()){
    (*iter)->setRFDesired(Fr_des);
    ++iter;
  }

  kin_wbc_ = new KinWBC<T>(cheetah::dim_config);
  wblc_ = new WBLC<T>(cheetah::dim_config, Ctrl::_contact_list);
  wblc_data_ = new WBLC_ExtraData<T>();

  for(size_t i(0); i<Ctrl::_contact_list.size(); ++i){
    _dim_contact += Ctrl::_contact_list[i]->getDim();
  }

  wblc_data_->W_qddot_ = DVec<T>::Constant(cheetah::dim_config, Weight::qddot_relax);
  wblc_data_->W_qddot_.head(6) = DVec<T>::Constant(6, Weight::qddot_relax_virtual);
  //wblc_data_->W_qddot_[0] = 1300.;
  //wblc_data_->W_qddot_[1] = 1300.;
  wblc_data_->W_qddot_[5] = 100.;
  wblc_data_->W_rf_ = DVec<T>::Constant(_dim_contact, Weight::tan_small);
  wblc_data_->W_xddot_ = DVec<T>::Constant(_dim_contact, Weight::foot_big);

  int idx_offset(0);
  for(size_t i(0); i<Ctrl::_contact_list.size(); ++i){
    //wblc_data_->W_rf_[idx_offset + Ctrl::_contact_list[i]->getFzIndex()] =
      //Weight::nor_small;
    idx_offset += Ctrl::_contact_list[i]->getDim();
  }

  // torque limit default setting
  wblc_data_->tau_min_ = DVec<T>::Constant(cheetah::num_act_joint, -150.);
  wblc_data_->tau_max_ = DVec<T>::Constant(cheetah::num_act_joint, 150.);

  _sp = StateProvider<T>::getStateProvider();

  printf("[WBLC_Two Contact Transition Ctrl] Constructed\n");
}

template <typename T>
WBLC_TwoContactTransCtrl<T>::~WBLC_TwoContactTransCtrl(){
  delete wblc_;
  delete kin_wbc_;
  delete wblc_data_;

  typename std::vector<Task<T>*>::iterator iter = Ctrl::_task_list.begin();
  while(iter < Ctrl::_task_list.end()){
    delete (*iter);
    ++iter;
  }
  Ctrl::_task_list.clear();

  typename std::vector<ContactSpec<T>*>::iterator iter2 = Ctrl::_contact_list.begin();
  while(iter2 < Ctrl::_contact_list.end()){
    delete (*iter2);
    ++iter2;
  }
  Ctrl::_contact_list.clear();
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::OneStep(void* _cmd){
  Ctrl::_PreProcessing_Command();
  Ctrl::_state_machine_time = _sp->_curr_time - ctrl_start_time_;

  _stiffness_gain_adjust = 1. + (_trot_test->_body_vel).norm()/1. ;
  _stiffness_gain_adjust = coerce<T>(_stiffness_gain_adjust, 1.0, 1.7);

  DVec<T> gamma;
  _contact_setup();
  _task_setup();
  _compute_torque_wblc(gamma);

  for(size_t leg(0); leg<cheetah::num_leg; ++leg){
    for(size_t jidx(0); jidx<cheetah::num_leg_joint; ++jidx){
      ((LegControllerCommand<T>*)_cmd)[leg].tauFeedForward[jidx] 
        = gamma[cheetah::num_leg_joint * leg + jidx];

      ((LegControllerCommand<T>*)_cmd)[leg].qDes[jidx] = 
        des_jpos_[cheetah::num_leg_joint * leg + jidx];

      ((LegControllerCommand<T>*)_cmd)[leg].qdDes[jidx] = 
        des_jvel_[cheetah::num_leg_joint * leg + jidx];

      ((LegControllerCommand<T>*)_cmd)[leg].kpJoint(jidx, jidx) = _Kp_joint[jidx];
      ((LegControllerCommand<T>*)_cmd)[leg].kdJoint(jidx, jidx) = _Kd_joint[jidx];
    }
  }
  Ctrl::_PostProcessing_Command();
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::_compute_torque_wblc(DVec<T> & gamma){
  // WBLC
  wblc_->UpdateSetting(Ctrl::_A, Ctrl::_Ainv, Ctrl::_coriolis, Ctrl::_grav);
  DVec<T> des_jacc_cmd = des_jacc_ 
    + Kp_.cwiseProduct(des_jpos_ - Ctrl::_robot_sys->_state.q)
    + Kd_.cwiseProduct(des_jvel_ - Ctrl::_robot_sys->_state.qd);

  wblc_data_->_des_jacc_cmd = des_jacc_cmd;
  wblc_->MakeTorque(gamma, wblc_data_);

  //pretty_print(wblc_data_->Fr_, std::cout, "reaction force");
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::_task_setup(){
  des_jpos_ = Ctrl::_robot_sys->_state.q;
  des_jvel_.setZero();
  des_jacc_.setZero();

  // Calculate IK for a desired height and orientation.
  Vec3<T> pos_des; pos_des.setZero();
  DVec<T> vel_des(3); vel_des.setZero();
  DVec<T> acc_des(3); acc_des.setZero();
  Vec3<T> rpy_des; rpy_des.setZero();
  DVec<T> ang_vel_des(_body_ori_task->getDim()); ang_vel_des.setZero();

  for(size_t i(0); i<3; ++i){
    pos_des[i] = _trot_test->_body_pos[i];
    vel_des[i] = _trot_test->_body_vel[i];
    acc_des[i] = _trot_test->_body_acc[i];

    rpy_des[i] = _trot_test->_body_ori_rpy[i];
    // TODO : Frame must coincide. Currently, it's not
    ang_vel_des[i] = _trot_test->_body_ang_vel[i];
  }

  _body_pos_task->UpdateTask(&(pos_des), vel_des, acc_des);

  // Set Desired Orientation
  Quat<T> des_quat; des_quat.setZero();
  des_quat = ori::rpyToQuat(rpy_des);

  DVec<T> ang_acc_des(_body_ori_task->getDim()); ang_acc_des.setZero();
  _body_ori_task->UpdateTask(&(des_quat), ang_vel_des, ang_acc_des);

  kin_wbc_->FindConfiguration(_sp->_Q, 
      Ctrl::_task_list, Ctrl::_contact_list, 
      des_jpos_, des_jvel_, des_jacc_);

  if(_transit_dir<0){
    T alpha(Ctrl::_state_machine_time/end_time_); //0->1
    des_jpos_ = alpha * des_jpos_ + (1.-alpha) * _trot_test->_jpos_des_pre;
  }
  //pretty_print(des_jpos_, std::cout, "des_jpos");
  //pretty_print(des_jvel_, std::cout, "des_jvel");
  //pretty_print(des_jacc_, std::cout, "des_jacc");
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::_contact_setup(){
  T alpha = 0.5 * (1-cos(M_PI * Ctrl::_state_machine_time/end_time_)); // 0 -> 1
  T upper_lim(100.);
  T rf_weight(100.);
  T rf_weight_z(100.);
  T foot_weight(1000.);

  if(_transit_dir > 0){ // Decrease reaction force & Increase full acceleration
    upper_lim = max_rf_z_ + alpha*(min_rf_z_ - max_rf_z_);
    rf_weight   = (1.-alpha)*Weight::tan_small  + alpha*Weight::tan_big;
    rf_weight_z = (1.-alpha)*Weight::nor_small + alpha*Weight::nor_big;
    foot_weight = (1.-alpha)*Weight::foot_big   + alpha*Weight::foot_small;
  } else {
    upper_lim = min_rf_z_ + alpha*(max_rf_z_ - min_rf_z_);
    rf_weight   = (1.-alpha)*Weight::tan_big  + alpha*Weight::tan_small;
    rf_weight_z = (1.-alpha)*Weight::nor_big + alpha*Weight::nor_small;
    foot_weight = (1.-alpha)*Weight::foot_small + alpha*Weight::foot_big;
  }

  if(_cp1 == linkID::FR || _cp2 == linkID::FR){
    _SetContact(0, upper_lim, rf_weight, rf_weight_z, foot_weight);
  }
  if(_cp1 == linkID::FL || _cp2 == linkID::FL){
    _SetContact(1, upper_lim, rf_weight, rf_weight_z, foot_weight);
  }

  if(_cp1 == linkID::HR || _cp2 == linkID::HR){
    _SetContact(2, upper_lim, rf_weight, rf_weight_z, foot_weight);
  }

  if(_cp1 == linkID::HL || _cp2 == linkID::HL){
    _SetContact(3, upper_lim, rf_weight, rf_weight_z, foot_weight);
  }


  typename std::vector<ContactSpec<T> *>::iterator iter = Ctrl::_contact_list.begin();
  while(iter < Ctrl::_contact_list.end()){
    (*iter)->UpdateContactSpec();
    ++iter;
  }
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::_SetContact(const size_t & cp_idx, 
    const T & upper_lim, const T & rf_weight, const T & rf_weight_z, const T & foot_weight){

  ((SingleContact<T>*)Ctrl::_contact_list[cp_idx])->setMaxFz(upper_lim);
  for(size_t i(0); i<3; ++i){
    wblc_data_->W_rf_[i + 3*cp_idx] = rf_weight;
    wblc_data_->W_xddot_[i + 3*cp_idx] = foot_weight;
  }
  wblc_data_->W_rf_[2 + 3*cp_idx] = rf_weight_z;
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::FirstVisit(){
  ctrl_start_time_ = _sp->_curr_time;
  ini_body_pos_ = Ctrl::_robot_sys->_state.bodyPosition;
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::LastVisit(){
  // printf("[ContactTransBody] End\n");
}

template <typename T>
bool WBLC_TwoContactTransCtrl<T>::EndOfPhase(){
  if(Ctrl::_state_machine_time > (end_time_-2.*Test<T>::dt)){
    return true;
  }
  return false;
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::CtrlInitialization(const std::string & category_name){
  //ParamHandler handler(CheetahConfigPath + setting_file_name + ".yaml");
  _param_handler->getValue<T>(category_name, "max_rf_z", max_rf_z_);
  _param_handler->getValue<T>(category_name, "min_rf_z", min_rf_z_);
}

template <typename T>
void WBLC_TwoContactTransCtrl<T>::SetTestParameter(const std::string & test_file){
  _param_handler = new ParamHandler(test_file);
  if(_param_handler->getValue<T>("body_height", _body_height_cmd)){
    b_set_height_target_ = true;
  }
  _param_handler->getValue<T>("transition_time", end_time_);

  // Feedback Gain
  std::vector<T> tmp_vec;
  _param_handler->getVector<T>("Kp", tmp_vec);
  for(size_t i(0); i<tmp_vec.size(); ++i){
    Kp_[i] = tmp_vec[i];
  }
  _param_handler->getVector<T>("Kd", tmp_vec);
  for(size_t i(0); i<tmp_vec.size(); ++i){
    Kd_[i] = tmp_vec[i];
  }
  // Feedback gain for kinematic tasks
  _param_handler->getVector<T>("Kp_body_pos_kin", tmp_vec);
  for(size_t i(0); i<_body_pos_task->getDim(); ++i){
    ((BodyPosTask<T>*)_body_pos_task)->_Kp_kin[i] = tmp_vec[i];
  }
  _param_handler->getVector<T>("Kp_body_ori_kin", tmp_vec);
  for(size_t i(0); i<_body_ori_task->getDim(); ++i){
    ((BodyOriTask<T>*)_body_ori_task)->_Kp_kin[i] = tmp_vec[i];
  }

  // torque limit default setting
  _param_handler->getVector<T>("tau_lim", tmp_vec);
  wblc_data_->tau_min_ = DVec<T>::Constant(cheetah::num_act_joint, tmp_vec[0]);
  wblc_data_->tau_max_ = DVec<T>::Constant(cheetah::num_act_joint, tmp_vec[1]);

  // Joint level feedback gain
  _param_handler->getVector<T>("Kp_joint", _Kp_joint);
  _param_handler->getVector<T>("Kd_joint", _Kd_joint);

}


template class WBLC_TwoContactTransCtrl<double>;
template class WBLC_TwoContactTransCtrl<float>;
