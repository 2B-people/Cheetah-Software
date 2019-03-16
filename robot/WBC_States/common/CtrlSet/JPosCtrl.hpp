#ifndef JPOS_CTRL_VALKYRIE
#define JPOS_CTRL_VALKYRIE

#include <WBC_States/Controller.hpp>
#include <WBC_States/Cheetah_DynaCtrl_Definition.h>
#include <ParamHandler/ParamHandler.hpp>

template <typename T> class ContactSpec;
template <typename T> class WBLC;
template <typename T> class WBLC_ExtraData;
template <typename T> class KinWBC;
template <typename T> class StateProvider;

namespace MotionType{
    constexpr int stay = 0;
    constexpr int swing = 1;
    constexpr int move_to_target = 2;
};

template <typename T>
class JPosCtrl: public Controller<T>{
    public:
        JPosCtrl(const FloatingBaseModel<T>* );
        virtual ~JPosCtrl();

        virtual void OneStep(void* _cmd);
        virtual void FirstVisit();
        virtual void LastVisit();
        virtual bool EndOfPhase();

        virtual void CtrlInitialization(const std::string & category_name);
        virtual void SetTestParameter(const std::string & test_file);

    protected:
        int _motion_type;
        DVec<T> _Kp, _Kd;

        DVec<T> _des_jpos; 
        DVec<T> _des_jvel; 
        DVec<T> _des_jacc;

        DVec<T> _jpos_ini;
        DVec<T> _jpos_target;

        std::vector<T> _swing_amp;
        std::vector<T> _swing_phase;
        std::vector<T> _swing_freq;

        T _end_time;
        int _dim_contact;
        T _ctrl_start_time;
        
        ContactSpec<T>* _contact;
        WBLC<T>* _wblc;
        WBLC_ExtraData<T>* _wblc_data;


        void _task_setup();
        void _contact_setup();
        void _compute_torque_wblc(DVec<T> & gamma);

        ParamHandler* _handler = NULL;
        StateProvider<T>* _sp;
        std::string _test_file_name;
};

#endif
