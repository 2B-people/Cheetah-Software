#ifndef BODY_CONTROL_TEST_Cheetah
#define BODY_CONTROL_TEST_Cheetah

#include <WBC_States/Test.hpp>
#include <Dynamics/FloatingBaseModel.h>


enum BodyCtrlPhase{
  BDCTRL_body_up_ctrl = 0,
  BDCTRL_body_ctrl = 1,
  NUM_BDCTRL_PHASE
};

template <typename T>
class BodyCtrlTest: public Test<T>{
public:
  BodyCtrlTest(FloatingBaseModel<T>* , const RobotType & );
  virtual ~BodyCtrlTest();

protected:
  virtual void _TestInitialization();
  virtual void _UpdateExtraData(Cheetah_Extra_Data<T> * ext_data);
  virtual int _NextPhase(const int & phase);
  void _SettingParameter();
  
  Controller<T>* body_up_ctrl_;
  Controller<T>* body_ctrl_;
};

#endif
