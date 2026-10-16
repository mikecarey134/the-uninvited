#include "CharacterProfile.h"
#include "net/ByteStream.h"

namespace character
{
	namespace
	{
		const size_t MAX_TEXT = 32;
		void (CharacterSheet::*const skillSetters[SKILL_COUNT])(int) = { &CharacterSheet::setSmithing, &CharacterSheet::setChemistry,
			&CharacterSheet::setMedicine, &CharacterSheet::setCooking, &CharacterSheet::setSurvivalism, &CharacterSheet::setLockpicking,
			&CharacterSheet::setElectronics, &CharacterSheet::setEngineering, &CharacterSheet::setSpeech, &CharacterSheet::setConstruction };
	}

	std::vector<uint8_t> packProfile(const CharacterSheet& sheet)
	{
		ByteWriter out;
		const Aegis& aegis = sheet.aegis();
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			out.writeU16((uint16_t)aegis.rolled[i]);
			out.writeU16((uint16_t)aegis.placed[i]);
		}
		out.writeU8(aegis.hasBloodline ? 1 : 0);
		out.writeU8((uint8_t)aegis.bloodlineAttribute);
		out.writeString(aegis.bloodline, MAX_TEXT);
		out.writeString(sheet.profession().name, MAX_TEXT);
		out.writeU8((uint8_t)sheet.profession().skillBonuses.size());
		for (const std::pair<int, int>& bonus : sheet.profession().skillBonuses)
		{
			out.writeU8((uint8_t)bonus.first);
			out.writeU16((uint16_t)bonus.second);
		}
		for (int i = 0; i < SKILL_COUNT; ++i)
			out.writeU16((uint16_t)sheet.skill(i));
		out.writeF32(sheet.age());
		return out.data();
	}

	bool unpackProfile(const std::vector<uint8_t>& profile, const std::string& name, Gender gender, int outfit, CharacterSheet& sheet)
	{
		ByteReader in(profile);
		Aegis aegis;
		for (int i = 0; i < ATTRIBUTE_COUNT; ++i)
		{
			aegis.rolled[i] = in.readU16();
			aegis.placed[i] = in.readU16();
		}
		aegis.hasBloodline = in.readU8() != 0;
		aegis.bloodlineAttribute = in.readU8() % ATTRIBUTE_COUNT;
		aegis.bloodline = in.readString(MAX_TEXT);
		Profession profession;
		profession.name = in.readString(MAX_TEXT);
		profession.skillBonuses.resize(in.readU8());
		for (std::pair<int, int>& bonus : profession.skillBonuses)
		{
			bonus.first = in.readU8() % SKILL_COUNT;
			bonus.second = in.readU16();
		}
		int skills[SKILL_COUNT];
		for (int& skill : skills)
			skill = in.readU16();
		float age = in.readF32();
		if (in.bad())
			return false;
		sheet = CharacterSheet::create(name, gender, profession, aegis);
		for (int i = 0; i < SKILL_COUNT; ++i)
			(sheet.*skillSetters[i])(skills[i]);
		sheet.setAge(age);
		sheet.setOutfit(outfit);
		return true;
	}
}
