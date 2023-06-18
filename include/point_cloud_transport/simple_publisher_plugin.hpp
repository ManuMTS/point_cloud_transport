// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: Czech Technical University in Prague .. 2019, paplhjak .. 2009, Willow Garage, Inc.

/*
 *
 * BSD 3-Clause License
 *
 * Copyright (c) Czech Technical University in Prague
 * Copyright (c) 2019, paplhjak
 * Copyright (c) 2009, Willow Garage, Inc.
 *
 *        All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 *        modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 *       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *       AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *       IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *       DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *       FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *       DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *       CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *       OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *       OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include <point_cloud_transport/publisher_plugin.hpp>
#include <point_cloud_transport/single_subscriber_publisher.hpp>

namespace point_cloud_transport
{

/**
 * \brief Base class to simplify implementing most plugins to Publisher.
 *
 * This base class vastly simplifies implementing a PublisherPlugin in the common
 * case that all communication with the matching SubscriberPlugin happens over a
 * single ROS topic using a transport-specific message type. SimplePublisherPlugin
 * is templated on the transport-specific message type and publisher dynamic
 * reconfigure type.
 *
 * A subclass need implement only two methods:
 * - getTransportName() from PublisherPlugin
 * - encodeTyped()
 *
 * For access to the parameter server and name remappings, use nh().
 *
 * getTopicToAdvertise() controls the name of the internal communication topic.
 * It defaults to \<base topic\>/\<transport name\>.
 *
 * \tparam M Type of the published messages.
 * \tparam Config Type of the publisher dynamic configuration.
 */
template<class M>
class SimplePublisherPlugin : public point_cloud_transport::SingleTopicPublisherPlugin
{
public:
  ~SimplePublisherPlugin() override
  {
  }

  uint32_t getNumSubscribers() const override
  {
    if (simple_impl_)
    {
      return simple_impl_->pub_->get_subscription_count();
    }
    return 0;
  }

  std::string getTopic() const override
  {
    if (simple_impl_)
    {
      return simple_impl_->pub_->get_topic_name();
    }
    return {};
  }

  void publish(const sensor_msgs::msg::PointCloud2& message) const override
  {
    if (!simple_impl_ || !simple_impl_->pub_)
    {
      auto logger = simple_impl_ ? simple_impl_->logger_ : rclcpp::get_logger("point_cloud_transport");
      RCLCPP_ERROR(
        logger,
        "Call to publish() on an invalid point_cloud_transport::SimplePublisherPlugin");
      return;
    }

    publish(message, bindInternalPublisher(simple_impl_->pub_.get()));
  }

  void shutdown() override
  {
    simple_impl_.reset();
  }

protected:
  virtual void advertiseImpl(
    rclcpp::Node * node, const std::string & base_topic,
    rmw_qos_profile_t custom_qos)
  {
    std::string transport_topic = getTopicToAdvertise(base_topic);
    simple_impl_ = std::make_unique<SimplePublisherPluginImpl>(node);

    RCLCPP_DEBUG(simple_impl_->logger_, "getTopicToAdvertise: %s", transport_topic.c_str());
    auto qos = rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(custom_qos), custom_qos);
    simple_impl_->pub_ = node->create_publisher<M>(transport_topic, qos);
  }

  //! Generic function for publishing the internal message type.
  typedef std::function<void (const M &)> PublishFn;

  /**
   * \brief Publish an pointcloud using the specified publish function. Must be implemented by
   * the subclass.
   *
   * The PublishFn publishes the transport-specific message type. This indirection allows
   * SimpleSubscriberPlugin to use this function for both normal broadcast publishing and
   * single subscriber publishing (in subscription callbacks).
   */
  virtual void publish(
    const sensor_msgs::msg::PointCloud2 & message,
    const PublishFn & publish_fn) const = 0;

  /**
   * \brief Return the communication topic name for a given base topic.
   *
   * Defaults to \<base topic\>/\<transport name\>.
   */
  virtual std::string getTopicToAdvertise(const std::string & base_topic) const
  {
    return base_topic + "/" + getTransportName();
  }

private:
  struct SimplePublisherPluginImpl
  {
    explicit SimplePublisherPluginImpl(rclcpp::Node * node)
    : node_(node),
      logger_(node->get_logger())
    {
    }

    rclcpp::Node * node_;
    rclcpp::Logger logger_;
    typename rclcpp::Publisher<M>::SharedPtr pub_;
  };

  std::unique_ptr<SimplePublisherPluginImpl> simple_impl_;

  typedef std::function<void (const sensor_msgs::msg::PointCloud2 &)> PointCloudPublishFn;

  /**
   * Returns a function object for publishing the transport-specific message type
   * through some ROS publisher type.
   *
   * @param pub An object with method void publish(const M&)
   */
  template<class PubT>
  PublishFn bindInternalPublisher(PubT * pub) const
  {
    // Bind PubT::publish(const Message&) as PublishFn
    typedef void (PubT::* InternalPublishMemFn)(const M &);
    InternalPublishMemFn internal_pub_mem_fn = &PubT::publish;
    return std::bind(internal_pub_mem_fn, pub, std::placeholders::_1);
  }
};

}