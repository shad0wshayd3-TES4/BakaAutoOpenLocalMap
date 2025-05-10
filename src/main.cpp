namespace Hooks
{
	class hkMapMenu :
		REX::Singleton<hkMapMenu>
	{
	private:
		static void SwitchTabsNotifyingAltar(void* a_this, std::int32_t a_id, void* a_target)
		{
			using func_t = decltype(&hkMapMenu::SwitchTabsNotifyingAltar);
			static REL::Relocation<func_t> func{ REL::Offset(0x658CE30) };
			return func(a_this, a_id, a_target);
		}

		static void StartFadeIn(void* a_this)
		{
			_StartFadeIn(a_this);

			if (auto Player = RE::PlayerCharacter::GetSingleton()) {
				if (Player->GetInterior()) {
					SwitchTabsNotifyingAltar(a_this, 1, nullptr);
				}
			}
		}

		inline static REL::HookVFT _StartFadeIn{ REL::Offset(0x86443A0), 24, StartFadeIn };
	};
}

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse);
	return true;
}
