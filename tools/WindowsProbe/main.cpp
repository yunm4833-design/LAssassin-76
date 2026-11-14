#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vstspeaker.h"
using namespace Steinberg;
using namespace Steinberg::Vst;
class Host final : public IHostApplication {
public:
 tresult PLUGIN_API queryInterface(const TUID iid,void** out) override {
  if(!std::memcmp(iid,FUnknown_iid,16)||!std::memcmp(iid,IHostApplication_iid,16)){*out=static_cast<IHostApplication*>(this);return kResultOk;}*out=nullptr;return kNoInterface;
 }
 uint32 PLUGIN_API addRef() override{return 1;}
 uint32 PLUGIN_API release() override{return 1;}
 tresult PLUGIN_API getName(String128 name) override{const char16_t s[]=u"LAssassin Probe";std::memcpy(name,s,sizeof(s));return kResultOk;}
 tresult PLUGIN_API createInstance(TUID,TUID,void** out) override{*out=nullptr;return kNotImplemented;}
};
static void stage(const char* s){std::printf("%s\n",s);std::fflush(stdout);}
int main(int argc,char** argv){
 if(argc<2){std::puts("Usage: LAssassinProbe.exe path-to-vst3-binary");return 2;}
 auto module=LoadLibraryA(argv[1]);if(!module){std::printf("LoadLibrary failed: Win32 error %lu\n",GetLastError());return 3;}stage("LoadLibrary OK");
 auto init=reinterpret_cast<bool(*)()>(GetProcAddress(module,"InitDll"));
 auto exit=reinterpret_cast<bool(*)()>(GetProcAddress(module,"ExitDll"));
 auto factoryFn=reinterpret_cast<IPluginFactory*(PLUGIN_API*)()>(GetProcAddress(module,"GetPluginFactory"));
 if(!init||!exit||!factoryFn){stage("Missing VST3 exports");return 4;}
 if(!init()){stage("InitDll failed");return 5;}stage("InitDll OK");auto* factory=factoryFn();if(!factory){stage("GetPluginFactory null");return 6;}
 PFactoryInfo info{};factory->getFactoryInfo(&info);std::printf("Vendor: %s; classes: %d\n",info.vendor,factory->countClasses());
 Host host;IComponent* component=nullptr;IAudioProcessor* audio=nullptr;
 for(int i=0;i<factory->countClasses();++i){PClassInfo cls{};factory->getClassInfo(i,&cls);std::printf("Class %d: %s [%s]\n",i,cls.name,cls.category);
 if(std::strcmp(cls.category,kVstAudioEffectClass)==0){auto result=factory->createInstance(cls.cid,IComponent_iid,reinterpret_cast<void**>(&component));std::printf("createInstance result=%ld\n",long(result));}}
 if(!component){stage("No audio component");return 7;}
 if(component->initialize(&host)!=kResultOk){stage("Component initialize failed");return 8;}stage("Component initialize OK");
 if(component->queryInterface(IAudioProcessor_iid,reinterpret_cast<void**>(&audio))!=kResultOk){stage("No IAudioProcessor");return 9;}
 SpeakerArrangement stereo=SpeakerArr::kStereo;
 if(audio->setBusArrangements(&stereo,1,&stereo,1)!=kResultOk){stage("Stereo bus setup failed");return 10;}
 ProcessSetup setup{kRealtime,kSample32,512,48000.};if(audio->setupProcessing(setup)!=kResultOk){stage("setupProcessing failed");return 11;}
 component->activateBus(kAudio,kInput,0,true);component->activateBus(kAudio,kOutput,0,true);component->setActive(true);audio->setProcessing(true);
 float inL[512]{},inR[512]{},outL[512]{},outR[512]{};float* inputs[]{inL,inR};float* outputs[]{outL,outR};AudioBusBuffers input{},output{};input.numChannels=output.numChannels=2;input.channelBuffers32=inputs;output.channelBuffers32=outputs;
 ProcessData data{};data.processMode=kRealtime;data.symbolicSampleSize=kSample32;data.numSamples=512;data.numInputs=data.numOutputs=1;data.inputs=&input;data.outputs=&output;
 double power=0;for(int block=0;block<100;++block){for(int i=0;i<512;++i)inL[i]=inR[i]=float(.2*std::sin(6.28318530718*997*(block*512+i)/48000.));if(audio->process(data)!=kResultOk){stage("process failed");return 12;}for(float sample:outL){if(!std::isfinite(sample)){stage("Nonfinite audio output");return 13;}power+=sample*sample;}}
 std::printf("Audio process OK; output RMS=%f; latency=%u\n",std::sqrt(power/51200),audio->getLatencySamples());
 TUID controllerID{};IEditController* controller=nullptr;component->getControllerClassId(controllerID);auto controllerResult=factory->createInstance(controllerID,IEditController_iid,reinterpret_cast<void**>(&controller));
 if(controllerResult!=kResultOk||!controller){stage("Controller creation failed");return 14;}
 if(controller->initialize(&host)!=kResultOk){stage("Controller initialize failed");return 15;}IConnectionPoint *a=nullptr,*b=nullptr;
 component->queryInterface(IConnectionPoint_iid,reinterpret_cast<void**>(&a));controller->queryInterface(IConnectionPoint_iid,reinterpret_cast<void**>(&b));
 if(a&&b){a->connect(b);b->connect(a);}
 std::printf("Controller OK; connected parameters=%d\n",controller->getParameterCount());
 if(a&&b){a->disconnect(b);b->disconnect(a);}if(a)a->release();if(b)b->release();
 controller->terminate();controller->release();audio->setProcessing(false);component->setActive(false);audio->release();component->terminate();component->release();factory->release();exit();FreeLibrary(module);stage("PASS: Windows VST3 load / scan / processing / controller");return 0;
}
