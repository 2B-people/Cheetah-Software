#ifndef WBLC_TWO_SWING_CHEETAH
#define WBLC_TWO_SWING_CHEETAH

#include <WBC_States/Controller.hpp>
#include <Utilities/BSplineBasic.h>

template <typename T> class ContactSpec;
template <typename T> class WBLC;
template <typename T> class WBLC_ExtraData;
template <typename T> class KinWBC;
template <typename T> class StateProvider;
template <typename T> class WBLCTrotTest;

template <typename T>
class WBLC_TwoLegSwingCtrl: public Controller<T>{
    public:
        WBLC_TwoLegSwingCtrl(WBLCTrotTest<T> * test, 
                const FloatingBaseModel<T>* , size_t cp1, size_t cp2);
        virtual ~WBLC_TwoLegSwingCtrl();

        virtual void OneStep(void* _cmd);
        virtual void FirstVisit();
        virtual void LastVisit();
        virtual bool EndOfPhase();

        virtual void CtrlInitialization(const std::string & setting_file_name);
        virtual void SetTestParameter(const std::string & test_file);
        void computeFootLoc(const Mat3<T> & Rot, const Vec3<T> & shoulder, 
                const T& step_time, 
                const Vec3<T> & body_pos, const Vec3<T> & body_vel, 
                const Vec3<T> & body_ang_vel, Vec3<T> & foot_loc);


    protected:
        T _step_time;
        void _GetSinusoidalSwingTrajectory(
                const Vec3<T> & ini, const Vec3<T> & fin, const T & t, 
                Vec3<T> & pos_des, DVec<T> & vel_des, DVec<T> & acc_des);

        void _GetBsplineSwingTrajectory(const T & t, BS_Basic<double, 3, 3, 1, 2, 2> & spline,
                Vec3<T> & pos_des, DVec<T> & vel_des, DVec<T> & acc_des);
        void _SetBspline(const Vec3<T> & st_pos, const Vec3<T> & des_pos, 
                BS_Basic<double, 3, 3, 1, 2, 2> & spline);
        void _SetContact(const size_t & cp_idx, const T & upper_lim, 
                const T & rf_weight, const T & rf_weight_z, const T & foot_weight);

        WBLCTrotTest<T>* _trot_test;
        size_t _cp1, _cp2;
        Vec3<T> _default_target_foot_loc_1;
        Vec3<T> _default_target_foot_loc_2;
        Vec3<T> _landing_offset;
        T _swing_height;
        Vec3<T> _prev_ori_command;

        Task<T>* _cp_pos_task1;
        Task<T>* _cp_pos_task2;

        Vec3<T> _foot_pos_ini1;
        Vec3<T> _target_loc1;
        Vec3<T> _foot_pos_des1;
        DVec<T> _foot_vel_des1;
        DVec<T> _foot_acc_des1;

        Vec3<T> _foot_pos_ini2;
        Vec3<T> _target_loc2;
        Vec3<T> _foot_pos_des2;
        DVec<T> _foot_vel_des2;
        DVec<T> _foot_acc_des2;

        Task<T>* body_pos_task_;
        Task<T>* body_ori_task_;

        ContactSpec<T>* fr_contact_;
        ContactSpec<T>* fl_contact_;
        ContactSpec<T>* hr_contact_;
        ContactSpec<T>* hl_contact_;

        KinWBC<T>* kin_wbc_;
        WBLC<T>* wblc_;
        WBLC_ExtraData<T>* wblc_data_;
        std::vector<ContactSpec<T>* > _kin_contact_list;

        DVec<T> base_pos_ini_;
        Vec3<T> ini_base_pos_;

        DVec<T> Kp_;
        DVec<T> Kd_;
        
        DVec<T> des_jpos_;
        DVec<T> des_jvel_;
        DVec<T> des_jacc_;

        T _end_time;
        T _target_body_height;
        T ini_body_height_;
        Vec3<T> _ini_body_pos;
        Vec3<T> _ini_body_target;
        
        T max_rf_z_;
        T min_rf_z_;
        int dim_contact_;
        T ctrl_start_time_;

        void _task_setup();
        void _contact_setup();
        void _compute_torque_wblc(DVec<T> & gamma);

        StateProvider<T>* _sp;
        T _dir_command[2];
        std::string _test_file_name;
        BS_Basic<double, 3, 3, 1, 2, 2> _foot_traj_1;
        BS_Basic<double, 3, 3, 1, 2, 2> _foot_traj_2;
};
#endif
