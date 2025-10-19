// RollingAverage.h
#pragma once
#include <Arduino.h>

/**
 * @brief RollingAverage maintains a running average of recent samples
 *        collected within a given time window (in milliseconds).
 *
 * The class stores up to 'capacity' samples in a ring buffer. When new
 * samples are added, old ones that fall outside the specified window
 * (based on millis()) are automatically discarded.
 *
 * Example:
 *   RollingAverage ra(128, 1000);  // 128-sample buffer, 1-second window
 *   ra.add(analogRead(A0));
 *   float avg = ra.average();       // get average of last 1s of data
 */
class RollingAverage
{
  public:
    /**
     * @param capacity  Maximum number of stored samples.
     * @param window_ms Time window in milliseconds over which to average.
     */
    RollingAverage(uint16_t capacity, uint32_t window_ms = 1000)
        : cap_(capacity ? capacity : 1), window_(window_ms)
    {
      buf_ = new Sample[cap_];
    }

    /** Destructor — frees allocated memory. */
    ~RollingAverage() { delete[] buf_; }

    /** Clear all stored samples and reset state. */
    inline void clear()
    {
      head_ = tail_ = count_ = 0;
      sum_ = 0.0f;
    }

    /**
     * @brief Add a new sample value.
     *
     * This automatically discards any samples older than window_ms.
     * If the buffer is full, the oldest sample is overwritten.
     *
     * @param v The sample value to add.
     */
    inline void add(float v)
    {
      const uint32_t now = millis();
      prune(now);

      // Drop oldest if buffer full
      if (count_ == cap_) {
        sum_ -= buf_[head_].v;
        head_ = next(head_);
        --count_;
      }

      buf_[tail_] = {v, now};
      tail_ = next(tail_);
      ++count_;
      sum_ += v;
    }

    /**
     * @brief Compute the current average of samples within the time window.
     *
     * @param fast If true (default), use the precomputed running sum (fast).
     *             If false, recalculate the sum from scratch (slower but accurate
     *             if you've modified data manually or want to verify integrity).
     * @return The average value, or 0.0f if no valid samples exist.
     */
    inline float average(bool fast = false)
    {
      prune(millis());
      if (!count_)
        return 0.0f;

      if (fast) {
        return sum_ / count_;
      } else {
        float total = 0.0f;
        for (uint16_t i = 0, idx = head_; i < count_; ++i) {
          total += buf_[idx].v;
          idx = next(idx);
        }
        return total / count_;
      }
    }

    /**
     * @brief Get the most recent sample value.
     *
     * @return The latest value, or 0.0f if no samples exist.
     */
    inline float latest()
    {
      prune(millis());
      if (!count_)
        return 0.0f;
      const uint16_t last = (tail_ == 0) ? (cap_ - 1) : (tail_ - 1);
      return buf_[last].v;
    }

    /**
     * @brief Get the number of valid samples currently inside the time window.
     */
    inline uint16_t count()
    {
      prune(millis());
      return count_;
    }

  private:
    struct Sample {
        float v;    ///< Sample value
        uint32_t t; ///< Timestamp in milliseconds
    };

    Sample* buf_ = nullptr;
    uint16_t cap_ = 0;    ///< Max samples
    uint16_t head_ = 0;   ///< Index of oldest sample
    uint16_t tail_ = 0;   ///< Index for next write
    uint16_t count_ = 0;  ///< Current sample count
    float sum_ = 0.0f;    ///< Running sum for fast average
    uint32_t window_ = 0; ///< Time window in ms

    inline uint16_t next(uint16_t i) const { return (i + 1u == cap_) ? 0u : (i + 1u); }

    /**
     * @brief Remove samples older than the time window.
     *
     * Uses unsigned subtraction so millis() wraparound is handled correctly.
     */
    inline void prune(uint32_t now)
    {
      while (count_) {
        const Sample& s = buf_[head_];
        if ((uint32_t)(now - s.t) <= window_)
          break;
        sum_ -= s.v;
        head_ = next(head_);
        --count_;
      }
    }
};