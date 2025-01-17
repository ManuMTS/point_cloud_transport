// Copyright (c) 2023 Open Source Robotics Foundation, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the Willow Garage nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <gtest/gtest.h>

#include <string>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include <sensor_msgs/msg/point_cloud2.hpp>

#include "point_cloud_transport/point_cloud_transport.hpp"

class TestSubscriber : public ::testing::Test
{
protected:
  void SetUp()
  {
    node_ = rclcpp::Node::make_shared("test_subscriber");
  }

  rclcpp::Node::SharedPtr node_;
};

TEST_F(TestSubscriber, construction_and_destruction)
{
  std::function<void(const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)> fcn =
    [](const auto & msg) {
      (void)msg;
    };

  auto sub = point_cloud_transport::create_subscription(node_, "pointcloud", fcn, "raw");

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.spin_node_some(node_);
}

TEST_F(TestSubscriber, shutdown)
{
  std::function<void(const sensor_msgs::msg::PointCloud2::ConstSharedPtr & msg)> fcn =
    [](const auto & msg) {(void)msg;};

  auto sub = point_cloud_transport::create_subscription(node_, "pointcloud", fcn, "raw");
  EXPECT_EQ(node_->get_node_graph_interface()->count_subscribers("pointcloud"), 1u);
  sub.shutdown();
  EXPECT_EQ(node_->get_node_graph_interface()->count_subscribers("pointcloud"), 0u);
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return ret;
}
