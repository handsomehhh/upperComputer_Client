

#include "app.h"
#include <iostream>

int main(int argc, char *argv[])
{
  std::cout<<"enter qingzhou_cloud node\n";
  ros::init(argc, argv, "qingzhou_cloud");
  ros::NodeHandle nh;
  app node(nh);
  node.run();
  return 0;
}

