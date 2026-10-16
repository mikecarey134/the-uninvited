#pragma once

namespace menu
{
	class Autosave
	{
	public:
		static constexpr float INTERVAL_SECONDS = 60.0f;

		bool due(float seconds)
		{
			unsaved_ += seconds;
			if (unsaved_ < INTERVAL_SECONDS)
				return false;
			unsaved_ = 0.0f;
			return true;
		}

	private:
		float unsaved_ = 0.0f;
	};
}
