#ifndef BODY_CTRL
#define BODY_CTRL

#include <WBC_state/Controller.hpp>
#include <ParamHandler/ParamHandler.hpp>

template <typename T> class ContactSpec;
template <typename T> class WBLC;
template <typename T> class WBLC_ExtraData;
template <typename T> class KinWBC;
template <typename T> class Cheetah_StateProvider;

template <typename T>
class BodyCtrl: public Controller<T>{
    public:
        BodyCtrl(const FloatingBaseModel<T>* );
        virtual ~BodyCtrl();

        virtual void OneStep(void* _cmd);
        virtual void FirstVisit();
        virtual void LastVisit();
        virtual bool EndOfPhase();

        virtual void CtrlInitialization(const std::string & category_name);
        virtual void SetTestParameter(const std::string & test_file);

        void SetStanceTime(const T & t){ end_time_ = t; }

    protected:
        bool _b_joystick_ctrl_on;
        DVec<T> Kp_, Kd_;
        DVec<T> des_jpos_; 
        DVec<T> des_jvel_; 
        DVec<T> des_jacc_;

        DVec<T> jpos_ini_;
        bool b_set_height_target_;
        T end_time_;
        int dim_contact_;

        std::vector<int> selected_jidx_;
        Task<T>* body_pos_task_;
        Task<T>* body_ori_task_;

        KinWBC<T>* kin_wbc_;
        ContactSpec<T>* fr_contact_;
        ContactSpec<T>* fl_contact_;
        ContactSpec<T>* hr_contact_;
        ContactSpec<T>* hl_contact_;
        WBLC<T>* wblc_;
        WBLC_ExtraData<T>* wblc_data_;

        T target_body_height_;
        T ini_body_height_;
        Vec3<T> ini_body_pos_;

        void _task_setup();
        void _contact_setup();
        void _compute_torque_wblc(DVec<T> & gamma);

        T ctrl_start_time_;
        ParamHandler* _param_handler;
        Cheetah_StateProvider<T>* _sp;
};

#endif
