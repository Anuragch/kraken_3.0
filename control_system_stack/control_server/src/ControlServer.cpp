#include <control_server/ControlServer.h>

namespace kraken_controller
{
  ControlServer::ControlServer(float freq)
  {
    ros::NodeHandle n;
    _time = n.createTimer(ros::Duration(1.0/freq),&ControlServer::timeCallBack,this);
    _pub = n.advertise<kraken_msgs::thrusterData4Thruster>("/kraken/thrusterData4Thruster",2);
    _sub_pose = n.subscribe<kraken_msgs::krakenPose>("/kraken/pose_estimated",2,&ControlServer::poseFeedBack,this);
    _sub_ip_error  = n.subscribe<kraken_msgs::ipControllererror>("/kraken/ip_error_data",2,&ControlServer::ipErrorFeedBack,this);
    _do_control = true;
    _ip_controller = false;
  }
  ControlServer::~ControlServer()
  {
    
  }
  
  void ControlServer::timeCallBack(const ros::TimerEvent &)
  {
    if(_do_control)
      {
        _controller.doControlIteration(_feedBack);
        _controller.updateState();
        _pub.publish(_controller.getThruster4Value());
      }
  }
  
  void ControlServer::setServers(actionlib::SimpleActionServer<kraken_msgs::advancedControllerAction> *ser1, actionlib::SimpleActionServer<kraken_msgs::controllerAction> *ser2)
  {
    _server1 = ser1;
    _server2 = ser2;
    _server1->start();
    _server2->start();
  }
  
  void ControlServer::poseFeedBack(const kraken_msgs::krakenPose::ConstPtr &msg)
  {
    
    if(!_ip_controller)
      {
        _feedBack.data[kraken_core::_px] = msg->data[kraken_core::_px];
        _feedBack.data[kraken_core::_py] = msg->data[kraken_core::_py];
        _feedBack.data[kraken_core::_pz] = msg->data[kraken_core::_pz];
      }
    for(int i=kraken_core::_pz+1;i<kraken_core::countState;i++)
      {
        _feedBack.data[i] = msg->data[i];
      }
    
  }
  
  void ControlServer::ipErrorFeedBack(const kraken_msgs::ipControllererror::ConstPtr &msg)
  {
    if(_ip_controller)
      {
        _feedBack.data[kraken_core::_px] = msg->dx;
        _feedBack.data[kraken_core::_py] = msg->dy;
        _feedBack.data[kraken_core::_pz] = msg->dz;
      }
  }
  
  bool ControlServer::moveAlongLine(kraken_msgs::moveAlongLine::Request &req, kraken_msgs::moveAlongLine::Response &res)
  {
    if(req.type==0)
      {
        _controller.pause();
      }
    else
      {
        if(req.angle<=2.0)
          {
            _controller.moveForward();
          }
        else
          {
            _controller.moveBack();
          }
      }
    return true;
  }
  
  bool ControlServer::changeController(kraken_msgs::switchControllers::Request &req, kraken_msgs::switchControllers::Response &res)
  {
    if(req.type==0)
      {
        _ip_controller = false;
      }
    else
      {
        _ip_controller = true;
      }
    return true;
  }
  
  void ControlServer::executePoseChange(const kraken_msgs::advancedControllerGoalConstPtr &msg)
  {
    
  }
  
  void ControlServer::executeOrientationChange(const kraken_msgs::controllerGoalConstPtr &msg)
  {
    _do_control = false;
    kraken_msgs::krakenPose _pose;
    _pose.data[kraken_core::_pz] = msg->d;
    _pose.data[kraken_core::_roll] = msg->r;
    _pose.data[kraken_core::_pitch] = msg->p;
    _pose.data[kraken_core::_yaw] = msg->y;
    _controller.setSetPoint(_pose);
    while(ros::ok())
    {
        if (_server2->isPreemptRequested() || !ros::ok())
        {
            _server2->setPreempted();
            break;
        }
        //_server2->publishFeedback(0);
    }
    //_server2->setSucceeded(0);
  }
  
  void ControlServer::loadParams(const std::vector<std::string> &filenames)
  {
    _controller.loadParams(filenames);
    _controller.pause();
  }
  
}
