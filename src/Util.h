//
// Created by thomas on 17.02.23.
//

#ifndef UTIL_H
#define UTIL_H

#include <mav/Message.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <thread>

class OutboundAdapter {
 private:
  std::function<void(const mav::ConnectionPartner&, mav::Message&)> _sendToGCS;
  std::function<void(mav::Message&)> _sendToSystem;
  std::function<void(mav::Message&)> _broadcastToGCS;
  std::function<void(const std::string&, mav::Message&)> _sendToGcsId;

 public:
  OutboundAdapter(std::function<void(const mav::ConnectionPartner&, mav::Message&)> send_to_gcs,
                  std::function<void(mav::Message&)> send_to_system,
                  std::function<void(mav::Message&)> broadcast_to_gcs,
                  std::function<void(const std::string&, mav::Message&)> send_to_gcs_id)
      : _sendToGCS(std::move(send_to_gcs)),
        _sendToSystem(std::move(send_to_system)),
        _broadcastToGCS(std::move(broadcast_to_gcs)),
        _sendToGcsId(std::move(send_to_gcs_id))
  {
  }

  void sendToGCS(const mav::ConnectionPartner& partner, mav::Message& message)
  {
    _sendToGCS(partner, message);
  }

  void sendToGcsId(const std::string& uuid, mav::Message& message) { _sendToGcsId(uuid, message); }

  void sendToSystem(mav::Message& message) { _sendToSystem(message); }

  void broadcastToGCS(mav::Message& message) { _broadcastToGCS(message); }
};

class Timer {
 private:
  std::atomic_bool _is_active{false};
  std::thread _timer_thread;
  static constexpr std::chrono::milliseconds DELAY_INTERVALL{10};

 public:
  Timer() = default;
  ~Timer() { stop(); }
  bool setInterval(const std::function<void()>& func, uint delay_ms)
  {
    if (!_is_active) {
      stop();
      _is_active = true;
      _timer_thread = std::thread{[=]() {
        while (_is_active) {
          std::chrono::time_point<std::chrono::system_clock> time_to_execute =
              std::chrono::system_clock::now() + std::chrono::milliseconds(delay_ms);
          while (std::chrono::duration_cast<std::chrono::milliseconds>(
                     time_to_execute - std::chrono::system_clock::now())
                     .count() > DELAY_INTERVALL.count()) {
            std::this_thread::sleep_for(DELAY_INTERVALL);
            if (!_is_active) return;
          }
          std::this_thread::sleep_until(time_to_execute);
          if (!_is_active) return;
          func();
        }
      }};

      return true;
    }

    return false;
  }
  bool setTimeout(const std::function<void()>& func, uint delay_ms)
  {
    if (!_is_active) {
      stop();
      _is_active = true;
      _timer_thread = std::thread{[=]() {
        std::chrono::time_point<std::chrono::system_clock> time_to_execute =
            std::chrono::system_clock::now() + std::chrono::milliseconds(delay_ms);
        if (!_is_active) return;
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   time_to_execute - std::chrono::system_clock::now())
                   .count() > DELAY_INTERVALL.count()) {
          std::this_thread::sleep_for(DELAY_INTERVALL);
          if (!_is_active) return;
        }
        std::this_thread::sleep_until(time_to_execute);
        if (!_is_active) return;
        func();
        _is_active = false;
      }};

      return true;
    }

    return false;
  }
  void stop()
  {
    _is_active = false;
    if (_timer_thread.joinable()) {
      _timer_thread.join();
    }
  }
};

class ExchangeTimeout {
 private:
  mutable std::mutex _mutex;
  std::chrono::time_point<std::chrono::system_clock> _last_value_received;
  mav::ConnectionPartner _partner;
  uint64_t _num_feedback_received{0UL};
  uint64_t _num_feedback_expected{0UL};
  double _timeout_delay{0.0};

  bool _isInProgress()
  {
    auto new_start = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = new_start - _last_value_received;
    return ((_num_feedback_received < _num_feedback_expected) && (diff.count() < _timeout_delay));
  };

 public:
  ExchangeTimeout() = default;
  bool tryNewConnection(mav::ConnectionPartner partner, double delay,
                        uint64_t num_feedback_expected = 1UL)
  {
    std::scoped_lock lock(_mutex);
    bool is_in_progress{_isInProgress()};
    if (!is_in_progress || partner == _partner) {
      _last_value_received = std::chrono::system_clock::now();
      _partner = partner;
      _timeout_delay = std::max(delay, 0.0);
      if (is_in_progress) {
        if (_num_feedback_expected > UINT64_MAX - num_feedback_expected) {
          _num_feedback_expected = UINT64_MAX;
        } else {
          _num_feedback_expected += num_feedback_expected;
        }

      } else {
        _num_feedback_expected = num_feedback_expected;
        _num_feedback_received = 0UL;
      }

      return true;
    }

    return false;
  }

  bool isInProgress()
  {
    std::scoped_lock lock(_mutex);
    return _isInProgress();
  }

  void addNewFeedback()
  {
    std::scoped_lock lock(_mutex);
    if (_isInProgress()) {
      _last_value_received = std::chrono::system_clock::now();
      _num_feedback_received++;
    }
  }

  void increaseExpectedFeedback()
  {
    std::scoped_lock lock(_mutex);
    if (_isInProgress()) {
      _num_feedback_expected++;
    }
  }

  const mav::ConnectionPartner& getLastConnectionPartner()
  {
    std::scoped_lock lock(_mutex);
    return _partner;
  }
};

#endif  // UTIL_H
