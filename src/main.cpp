#include <CoreAudio/AudioHardware.h>
#include <iostream>

void RegisterGlobalListeners();
void RegisterDeviceListeners(AudioDeviceID old_device,
                             AudioDeviceID new_device);
AudioDeviceID GetDefaultDeviceId();
void DumpDevice(AudioDeviceID id);

// Used to register for notifications when the default output device
// changes.
const AudioObjectPropertyAddress kDefaultOutputDeviceChangePropertyAddress = {
    kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster};

// Used to get the internal device ID of the default output device.
const AudioObjectPropertyAddress kDefaultOutputDevicePropertyAddress = {
    kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster};

// Used to get the name of an audio device.
const AudioObjectPropertyAddress kAudioDeviceNamePropertyAddress = {
    kAudioDevicePropertyDeviceNameCFString, kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster};

// Used to get output stream details such as sample rate and channel count.
const AudioObjectPropertyAddress kAudioDeviceStreamFormatPropertyAddress = {
    kAudioDevicePropertyStreamFormat, kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster};

// Properties that we want to watch on a specific audio device.
struct AudioProperty {
  const char *const name;
  const AudioObjectPropertyAddress addr;
};

static const AudioProperty kAudioDevicePropChanges[] = {
    {"DeviceHasChanged",
     {kAudioDevicePropertyDeviceHasChanged, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMaster}},
    {"StreamConfiguration",
     {kAudioDevicePropertyStreamConfiguration, kAudioObjectPropertyScopeOutput,
      kAudioObjectPropertyElementMaster}},
    {"NominalSampleRate",
     {kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeOutput,
      kAudioObjectPropertyElementMaster}},
    {"BufferFrameSize",
     {kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeOutput,
      kAudioObjectPropertyElementMaster}},
    {"BufferFrameSizeRange",
     {kAudioDevicePropertyBufferFrameSizeRange, kAudioObjectPropertyScopeOutput,
      kAudioObjectPropertyElementMaster}},
};

AudioDeviceID devid = kAudioObjectUnknown;

std::string ToString(const CFStringRef name) {
  CFIndex len = CFStringGetLength(name);
  CFIndex size =
      CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8) + 1;
  auto buff = (char *)calloc(size, 1);
  CFStringGetCString(name, buff, size, kCFStringEncodingUTF8);
  std::string ret(buff);
  free(buff);
  return ret;
}

OSStatus AudioDeviceCB(AudioObjectID inObjectID, UInt32 inNumberAddresses,
                       const AudioObjectPropertyAddress inAddresses[],
                       void *inClientData) {
  bool new_default = false;
  for (UInt32 i = 0; i < inNumberAddresses; i++) {
    const char *name;
    const auto &addr = inAddresses[i];
    switch (addr.mSelector) {
    case kAudioDevicePropertyDeviceHasChanged:
      name = "DeviceHasChanged";
      break;
    case kAudioDevicePropertyStreamConfiguration:
      name = "StreamConfiguration";
      break;
    case kAudioDevicePropertyNominalSampleRate:
      name = "NominalSampleRate";
      break;
    case kAudioHardwarePropertyDefaultOutputDevice:
      name = "DefaultOutputDevice";
      new_default = true;
      break;
    case kAudioDevicePropertyBufferFrameSize:
      name = "BufferFrameSize";
      break;
    case kAudioDevicePropertyBufferFrameSizeRange:
      name = "BufferFrameSizeRange";
      break;
    default:
      name = "unknonwn";
      break;
    }
    std::cout << "AudioDeviceCB: " << name << std::endl;
  }

  if (new_default) {
    auto old_id = devid;
    devid = GetDefaultDeviceId();
    DumpDevice(devid);
    RegisterDeviceListeners(old_id, devid);
  }

  return noErr;
}

void RegisterGlobalListeners() {
  AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                 &kDefaultOutputDeviceChangePropertyAddress,
                                 &AudioDeviceCB, NULL);
}

void RegisterDeviceListeners(AudioDeviceID old_device,
                             AudioDeviceID new_device) {
  if (old_device == new_device)
    return;

  std::cout << "Output device has changed: " << old_device << " => "
            << new_device << std::endl;

  if (old_device != kAudioObjectUnknown) {
    for (auto &obj : kAudioDevicePropChanges) {
      AudioObjectRemovePropertyListener(old_device, &obj.addr, &AudioDeviceCB,
                                        NULL);
    }
    std::cout << "Unregistered listeners for " << old_device << std::endl;
  }

  if (new_device != kAudioObjectUnknown) {
    for (auto &obj : kAudioDevicePropChanges) {
      AudioObjectAddPropertyListener(new_device, &obj.addr, &AudioDeviceCB,
                                     NULL);
      std::cout << "Registered " << obj.name << " listener for " << new_device
                << std::endl;
    }
  }
}

AudioDeviceID GetDefaultDeviceId() {
  AudioDeviceID devid = kAudioObjectUnknown;
  UInt32 size = sizeof(AudioDeviceID);
  AudioObjectGetPropertyData(kAudioObjectSystemObject,
                             &kDefaultOutputDevicePropertyAddress, 0, NULL,
                             &size, &devid);
  return devid;
}

void DumpDevice(AudioDeviceID id) {
  std::cout << "Device details (" << id << ")" << std::endl;

  CFStringRef name;
  UInt32 size = sizeof(name);
  AudioObjectGetPropertyData(id, &kAudioDeviceNamePropertyAddress, 0, 0, &size,
                             &name);
  std::cout << "  device name: " << ToString(name) << std::endl;

  // Query the stream format for the output device from CoreAudio.
  AudioStreamBasicDescription stream_format;
  size = sizeof(stream_format);
  AudioObjectGetPropertyData(id, &kAudioDeviceStreamFormatPropertyAddress, 0,
                             NULL, &size, &stream_format);
  std::cout << "  sample rate: " << stream_format.mSampleRate << std::endl
            << "  channels:    " << stream_format.mChannelsPerFrame
            << std::endl;
}

int main() {
  RegisterGlobalListeners();
  devid = GetDefaultDeviceId();
  DumpDevice(devid);
  RegisterDeviceListeners(kAudioObjectUnknown, devid);

  while (true) {
    sleep(100);
  }

  return 0;
}