/*! @file Graphics3D.h
 *  @brief Visualizer window for simulator
 *
 *  This class displays a window for 3D graphics. It also implements scroll/pan/zoom.
 *  This uses OpenGL and QT.
 */

#ifndef PROJECT_GRAPHICS3D_H
#define PROJECT_GRAPHICS3D_H

#include "obj_loader.h"
#include "DrawList.h"
#include "Math/FirstOrderIIRFilter.h"

#include <QWindow>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QPainter>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

#include <QWheelEvent>
#include <QScreen>
#include <QDateTime>
#include <QMouseEvent>

#include <mutex>
#include "GameController.h"

class Graphics3D: public QOpenGLWidget, protected QOpenGLFunctions {
Q_OBJECT
public:
  explicit Graphics3D(QWidget* parent = 0);
  virtual ~Graphics3D();

  void setAnimating(bool animating);
  size_t setupCheetah3();
  size_t setupMiniCheetah();

  void lockGfxMutex() {
    _gfxMutex.lock();
  }

  void unlockGfxMutex() {
    _gfxMutex.unlock();
  }

  // set robot state
  double _fps = 0;
  DrawList _drawList;
  char infoString[200] = "";

  GamepadCommand& getDriverCommand() {
    return _driverCommand;
  }

  void resetGameController() {
    _gameController.findNewController();
  }

  bool IsPaused(){ return _pause; }
protected:
  void initializeGL() override;
  //void resizeGL(int w, int h) override;
  void paintGL() override;
  //bool event(QEvent *event) override;

  //void exposeEvent(QExposeEvent *event) override; ??

  // mouse callbacks for orbit and zoom
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *e) override;
  void keyReleaseEvent(QKeyEvent *e) override;
  void keyPressEvent(QKeyEvent *event) override;

  float _color1[3] = {0.364784, 0.513401, 0.952230};
  float _color2[3] = {0.553970, 0.477397, 0.628871};
  float _color3[3] = {0.335223, 0.768230, 0.277775};


private:

  GameController _gameController;
  GamepadCommand _driverCommand;

  std::mutex _gfxMutex;
  void updateCameraMatrix();
  void _BoxObstacleDrawing();
  void _MeshObstacleDrawing();
  void _DrawBox( double depth, double width, double height);
  void _Additional_Drawing();
  void _DrawContactForce();
  void _DrawContactPoint();
  void _drawArrow(double x0, double y0, double z0, double dx, double dy, double dz ,double lineWidth, double headWidth, double headLength);
  bool _animating;

  // attributes for shader program
  GLuint _posAttr; // position of vertex
  GLuint _colAttr; // color of vertex
  GLuint _matrixUniform; // transformation matrix
  GLuint _normAttr; // vertex normal
  GLuint _textTexture = 0;

  // shader programs
  QOpenGLShaderProgram *_program;

  // frame count
  int _frame;
  // time of last frame
  qint64 last_frame_ms = 0;

  // UI orbit/zoom variables
  bool _orbiting = false;
  int _orbiting_x_start = 0;
  int _orbiting_y_start = 0;
  float _rx_base = 0;
  float _ry_base = 0;
  float _rx = 0;
  float _ry = -90;
  float _pixel_to_rad = .3f;
  float _zoom = 1;

  bool _rotOrig = true;

  QMatrix4x4 _cameraMatrix;
  Vec3<float> _v0;
  FirstOrderIIRFilter<Vec3<float>, float> _freeCamFilter;

  float _freeCamMove [3] = {0,0,0};
  float _freeCamPos [3] = {0.f,0.f,0.f};
  float _frameTime = 1.f/60.f;

  bool _arrowsPressed [4] = {false,false,false,false};

  float _targetSpeed = 2;

  float _r[8];
  float _g[8];
  float _b[8];


  void getHeightColor(const double & height, float& r, float& g, float&b);
  void _SetRGBHeight(const double & h, const double & step, const int & idx, 
          float & r, float & g, float & b);


  bool _pause = false;
};

#endif //PROJECT_GRAPHICS3D_H
