/*! @file FloatingBaseModel.h
 *  @brief Implementation of Rigid Body Floating Base model data structure
 *
 * This class stores the kinematic tree described in "Rigid Body Dynamics Algorithms" by Featherstone
 * (download from https://www.springer.com/us/book/9780387743141 on MIT internet)
 *
 * The tree includes an additional "rotor" body for each body.  This rotor is fixed to the parent body and
 * has a gearing constraint.  This is efficiently included using a technique similar to what is described in
 * Chapter 12 of "Robot and Multibody Dynamics" by Jain.  Note that this implementation is highly specific to the case
 * of a single rotating rotor per rigid body.  Rotors have the same joint type as their body, but with an additional
 * gear ratio multiplier applied to the motion subspace. The rotors associated with the floating base don't do anything.
 */

#ifndef LIBBIOMIMETICS_FLOATINGBASEMODEL_H
#define LIBBIOMIMETICS_FLOATINGBASEMODEL_H

#include "spatial.h"
#include "orientation_tools.h"
#include "SpatialInertia.h"

#include <eigen3/Eigen/StdVector>

#include <vector>
#include <string>

using std::vector;
using namespace ori;
using namespace spatial;

/*!
 * The state of a floating base model (base and joints)
 */
template <typename T>
struct FBModelState {
  Quat<T> bodyOrientation;
  Vec3<T> bodyPosition;
  SVec<T> bodyVelocity; // body coordinates
  DVec<T> q;
  DVec<T> qd;

  void print () const {
    printf("position: %.3f %.3f %.3f\n", bodyPosition[0], bodyPosition[1], bodyPosition[2]);
  }
};

/*!
 * The result of running the articulated body algorithm on a rigid-body floating base model
 */
template <typename T>
struct FBModelStateDerivative {
  Quat<T> dQuat;
  Vec3<T> dBodyPosition;
  SVec<T> dBodyVelocity;
  DVec<T> qdd;
};


/*!
 * Class to represent a floating base rigid body model with rotors and ground contacts. No concept of state.
 */
template<typename T>
class FloatingBaseModel {
public:

  /*!
   * Initialize a floating base model with default gravity
   */
  FloatingBaseModel() : _gravity(0,0,-9.81) {}

  /*!
   * Add floating base.  Must be the first body added, and there can only be one
   */
  void addBase(const SpatialInertia<T>& inertia);

  /*!
   * Add floating base.  Must be the first body added, and there can only be one
   */
  void addBase(T mass, const Vec3<T>& com, const Mat3<T>& I);

  /*!
   * Add a point for collisions
   * @param bodyID   : body that the point belongs to (body 5 for floating base)
   * @param location : location of point in body coordinates
   * @param isFoot   : if the point is a foot or not.  Only feet have their Jacobian calculated on the robot
   * @return collisionPointID of the new point
   */
  int addGroundContactPoint(int bodyID, const Vec3<T>& location, bool isFoot = false);

  /*!
   * Add bounding box collision points around a body.
   * @param bodyId : Body to add
   * @param dims   : Dimension of points
   */
  void addGroundContactBoxPoints(int bodyId, const Vec3<T>& dims);

  /*!
   * Add a body to the tree
   * @param inertia        : Inertia of body (body coords)
   * @param rotorInertia   : Inertia of rotor (rotor coords)
   * @param gearRatio      : Gear ratio.  >1 for a gear reduction
   * @param parent         : Body ID of the link that the body is connected to
   * @param jointType      : Type of joint
   * @param jointAxis      : Axis of joint
   * @param Xtree          : Location of joint
   * @param Xrot           : Location of rotor
   * @return               : bodyID
   */
  int addBody(const SpatialInertia<T>& inertia, const SpatialInertia<T>& rotorInertia, T gearRatio,
              int parent, JointType jointType, CoordinateAxis jointAxis, const Mat6<T>& Xtree, const Mat6<T>& Xrot);

  /*!
 * Add a body to the tree
 * @param inertia        : Inertia of body (body coords)
 * @param rotorInertia   : Inertia of rotor (rotor coords)
 * @param gearRatio      : Gear ratio.  >1 for a gear reduction
 * @param parent         : Body ID of the link that the body is connected to
 * @param jointType      : Type of joint
 * @param jointAxis      : Axis of joint
 * @param Xtree          : Location of joint
 * @param Xrot           : Location of rotor
 * @return               : bodyID
 */
  int addBody(const MassProperties<T>& inertia, const MassProperties <T>& rotorInertia, T gearRatio,
              int parent, JointType jointType, CoordinateAxis jointAxis, const Mat6<T>& Xtree, const Mat6<T>& Xrot);

  /*!
   * Very simple to check to make sure the dimensions are correct
   */
  void check();

  /*!
   * Total mass of all rotors
   */
  T totalRotorMass();

  /*!
   * Total mass of all bodies which are not rotors
   */
  T totalNonRotorMass();

  const std::vector<int>& getParentVector() {
    return _parents;
  }

  const std::vector<SpatialInertia<T>, Eigen::aligned_allocator<SpatialInertia<T>>>& getBodyInertiaVector() {
    return _Ibody;
  }

  const std::vector<SpatialInertia<T>, Eigen::aligned_allocator<SpatialInertia<T>>>& getRotorInertiaVector() {
    return _Irot;
  }

  /*!
   * Set the gravity
   */
  void setGravity(Vec3<T>& g) {
    _gravity = g;
  }

  
  void setContactComputeFlag(size_t gc_index, bool flag) {
    _compute_contact_info[gc_index] = flag;
  }


  void addDynamicsVars(int count);
  
  void resizeSystemMatricies();

  void setState( FBModelState<T> & state) {
    _state = state;
    _kinematicsUpToDate = false;
    _biasAccelerationsUpToDate = false;
    _compositeInertiasUpToDate = false;
  }

  void forwardKinematics();
  void biasAccelerations();
  void compositeInertias();

  void contactJacobians();
  
  DVec<T> gravityForce();
  DVec<T> coriolisForce();
  DMat<T> massMatrix();
  DVec<T> inverseDynamics(FBModelStateDerivative<T> & dState);


  size_t _nDof = 0;
  Vec3 <T> _gravity;
  vector<int> _parents;
  vector<T> _gearRatios;
  vector<JointType> _jointTypes;
  vector<CoordinateAxis> _jointAxes;
  vector<Mat6<T>, Eigen::aligned_allocator<Mat6<T>>> _Xtree, _Xrot;
  vector<SpatialInertia<T>, Eigen::aligned_allocator<SpatialInertia<T>>> _Ibody, _Irot;
  vector<std::string> _bodyNames;

  size_t _nGroundContact = 0;
  vector<size_t> _gcParent;
  vector<Vec3<T>> _gcLocation;
  vector<uint64_t> _footIndicesGC;

  vector< Vec3<T> > _pGC;
  vector< Vec3<T> > _vGC;

  vector< bool > _compute_contact_info;



  /// BEGIN ALGORITHM SUPPORT VARIABLES
  FBModelState<T> _state;

  vectorAligned< SVec<T> > _v, _vrot, _a, _arot, _avp,_avprot,  _c, _crot, _S, _Srot, _fvp, _fvprot, _ag, _agrot, _f, _frot;
  vectorAligned< SpatialInertia<T> >  _IC ;
  
  vectorAligned< Mat6<T> > _Xup, _Xa, _Xuprot;

  DMat<T> _H, _C;
  DVec<T> _Cqd, _G;

  vectorAligned< D6Mat<T> > _J;
  vectorAligned< SVec<T> > _Jdqd;

  vectorAligned< D3Mat<T> > _Jc;
  vectorAligned< Vec3<T> > _Jcdqd;


  bool  _kinematicsUpToDate = false;
  bool  _biasAccelerationsUpToDate = false;
  bool  _compositeInertiasUpToDate = false;

};


#endif //LIBBIOMIMETICS_FLOATINGBASEMODEL_H
