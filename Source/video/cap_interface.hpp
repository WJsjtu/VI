#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "utils.h"

#define CV_LOG_INFO(flag, stream) { std::ostringstream ss; ss << stream << std::endl; Utils::LoggerPrintf(Utils::LogLevel::Info, ss.str().c_str()); }
#define CV_LOG_DEBUG(flag, stream) { std::ostringstream ss; ss << stream << std::endl; Utils::LoggerPrintf(Utils::LogLevel::Info, ss.str().c_str()); }
#define CV_LOG_ERROR(flag, stream) { std::ostringstream ss; ss << stream << std::endl; Utils::LoggerPrintf(Utils::LogLevel::Error, ss.str().c_str()); }
#define CV_WARN(flag, stream) { std::ostringstream ss; ss << stream << std::endl; Utils::LoggerPrintf(Utils::LogLevel::Warning, ss.str().c_str()); }

struct IVideoCaptureFrame
{
	unsigned char* data;
	int step;
	int width;
	int height;
	int cn;
};

struct CvCapture
{
	virtual ~CvCapture() {}
	virtual double getProperty(int) const { return 0; }
	virtual bool setProperty(int, double) { return 0; }
	virtual bool grabFrame() { return true; }
	virtual IVideoCaptureFrame* retrieveFrame(int) { return 0; }
};


template <class T>
inline T castParameterTo(int paramValue)
{
	return static_cast<T>(paramValue);
}

template <>
inline bool castParameterTo(int paramValue)
{
	return paramValue != 0;
}

class VideoParameters
{
public:
	struct VideoParameter {
		VideoParameter() = default;

		VideoParameter(int key_, int value_) : key(key_), value(value_) {}

		int key{ -1 };
		int value{ -1 };
		mutable bool isConsumed{ false };
	};

	VideoParameters() = default;

	explicit VideoParameters(const std::vector<int>& params)
	{
		const auto count = params.size();
		if (count % 2 != 0)
		{
			throw std::runtime_error("Vector of VideoWriter parameters should have even length");
		}
		params_.reserve(count / 2);
		for (std::size_t i = 0; i < count; i += 2)
		{
			add(params[i], params[i + 1]);
		}
	}

	VideoParameters(int* params, unsigned n_params)
	{
		params_.reserve(n_params);
		for (unsigned i = 0; i < n_params; ++i)
		{
			add(params[2 * i], params[2 * i + 1]);
		}
	}

	void add(int key, int value)
	{
		params_.emplace_back(key, value);
	}

	bool has(int key) const
	{
		auto it = std::find_if(params_.begin(), params_.end(),
			[key](const VideoParameter &param)
		{
			return param.key == key;
		}
		);
		return it != params_.end();
	}

	template <class ValueType>
	ValueType get(int key) const
	{
		auto it = std::find_if(params_.begin(), params_.end(),
			[key](const VideoParameter &param)
		{
			return param.key == key;
		}
		);
		if (it != params_.end())
		{
			it->isConsumed = true;
			return castParameterTo<ValueType>(it->value);
		}
		else
		{
			throw std::runtime_error("Missing value for parameter: [" + std::to_string(key) + "]");
		}
	}

	template <class ValueType>
	ValueType get(int key, ValueType defaultValue) const
	{
		auto it = std::find_if(params_.begin(), params_.end(),
			[key](const VideoParameter &param)
		{
			return param.key == key;
		}
		);
		if (it != params_.end())
		{
			it->isConsumed = true;
			return castParameterTo<ValueType>(it->value);
		}
		else
		{
			return defaultValue;
		}
	}

	std::vector<int> getUnused() const
	{
		std::vector<int> unusedParams;
		for (const auto &param : params_)
		{
			if (!param.isConsumed)
			{
				unusedParams.push_back(param.key);
			}
		}
		return unusedParams;
	}

	std::vector<int> getIntVector() const
	{
		std::vector<int> vint_params;
		for (const auto& param : params_)
		{
			vint_params.push_back(param.key);
			vint_params.push_back(param.value);
		}
		return vint_params;
	}

	bool empty() const
	{
		return params_.empty();
	}

	bool warnUnusedParameters() const
	{
		bool found = false;
		for (const auto &param : params_)
		{
			if (!param.isConsumed)
			{
				found = true;
				CV_WARN(NULL, "VIDEOIO: unused parameter: [" << param.key << "]=" <<
					fmt::sprintf("%lld / 0x%016llx", (long long)param.value, (long long)param.value));
			}
		}
		return found;
	}


private:
	std::vector<VideoParameter> params_;
};

class VideoCaptureParameters : public VideoParameters
{
public:
	using VideoParameters::VideoParameters;  // reuse constructors
};

class IVideoCapture
{
public:
	virtual ~IVideoCapture() {}
	virtual double getProperty(int) const { return 0; }
	virtual bool setProperty(int, double) { return false; }
	virtual bool grabFrame() = 0;
	virtual bool retrieveFrame(int, IVideoCaptureFrame&) = 0;
	virtual bool isOpened() const = 0;
	virtual void seek(int64_t frame_number) = 0;
	virtual void seek(double sec) = 0;
};

IVideoCapture* cvCreateFileCapture_FFMPEG_proxy(const std::string &filename, const VideoCaptureParameters& params);
