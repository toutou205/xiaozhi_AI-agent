#ifndef WAKE_WORD_H
#define WAKE_WORD_H

#include <functional>
#include <string>
#include <vector>

#include "audio_codec.h"
#include <model_path.h>

class WakeWord {
public:
  virtual ~WakeWord() = default;

  virtual bool Initialize(AudioCodec *codec, srmodel_list_t *models_list) = 0;
  virtual void Feed(const std::vector<int16_t> &data) = 0;
  virtual void OnWakeWordDetected(
      std::function<void(const std::string &wake_word)> callback) = 0;
  virtual void Start() = 0;
  virtual void Stop() = 0;
  virtual size_t GetFeedSize() = 0;
  virtual void EncodeWakeWordData() = 0;
  virtual bool GetWakeWordOpus(std::vector<uint8_t> &opus) = 0;
  virtual const std::string &GetLastDetectedWakeWord() const = 0;
};

#endif
