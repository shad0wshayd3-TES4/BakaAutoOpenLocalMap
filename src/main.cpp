namespace REX::JSON
{
	using list_t = std::vector<std::string>;

	namespace Impl
	{
		template <>
		void SettingLoad<list_t>(
			void*   a_data,
			path_t  a_path,
			list_t& a_value,
			list_t& a_valueDefault)
		{
			const auto& json = *static_cast<nlohmann::json*>(a_data);
			if (a_path[0] == '/')
			{
				a_value = json.value<list_t>(nlohmann::json::json_pointer(a_path.data()), a_valueDefault);
			}
			else
			{
				a_value = json.value<list_t>(a_path, a_valueDefault);
			}
		}

		template <>
		void SettingSave<list_t>(
			void*   a_data,
			path_t  a_path,
			list_t& a_value)
		{
			auto& json = *static_cast<nlohmann::json*>(a_data);
			if (a_path[0] == '/')
			{
				json[nlohmann::json::json_pointer(a_path.data())] = a_value;
			}
			else
			{
				json[a_path] = a_value;
			}
		}
	}

	template <class Store = SettingStore>
	using StrA = Setting<list_t, Store>;
}

namespace JSON
{
	static REX::JSON::Bool AutoInteriors{ "autoInteriors", true };
	static REX::JSON::Bool AutoSmallWorld{ "autoSmallWorld", true };
	static REX::JSON::Bool DisableFogOfWar{ "disableFogOfWar", false };

	static REX::JSON::StrA AutoSmallWorldBlockList{
		"autoSmallWorldBlocklist",
		{}
	};

	static REX::JSON::StrA AutoWorldSpaces{
		"autoWorldSpaces",
		{ "SETheFringe"s,
          "SETheFringeOrdered"s,
          "SENSBliss"s,
          "SENSCrucible"s,
          "SENSPalace"s }
	};

	static void Init()
	{
		const auto json = REX::JSON::SettingStore::GetSingleton();
		json->Init(
			"OBSE/plugins/BakaAutoOpenLocalMap.json",
			"OBSE/plugins/BakaAutoOpenLocalMapCustom.json");
		json->Load();
	}
}

namespace HOOK
{
	class hkMapMenu :
		public REX::Singleton<hkMapMenu>
	{
	private:
		using handler_t = void(UE::VPairingEntry*, bool);

		static void SendHandler(UE::IPairingGate* a_this, UE::VPairingEntry* a_pairingEntry, std::uint64_t a_frameNumber, const char* a_handlerName, handler_t* a_handler, bool a_arg)
		{
			using func_t = decltype(&hkMapMenu::SendHandler);
			static REL::Relocation<func_t> func{ REL::Offset(0x47A8920) };
			return func(a_this, a_pairingEntry, a_frameNumber, a_handlerName, a_handler, a_arg);
		}

		static void HandleGetCanFastTravelFromWorldSpace(UE::VPairingEntry* a_pairingEntry, bool a_arg)
		{
			using func_t = decltype(&hkMapMenu::HandleGetCanFastTravelFromWorldSpace);
			static REL::Relocation<func_t> func{ REL::Offset(0x47338A0) };
			return func(a_pairingEntry, a_arg);
		}

		inline static REL::Relocation<std::uint32_t*> Main_iFrameCounter{ REL::Offset(0x8F9F538) };

	private:
		static bool ListHasWorldSpace(const std::vector<std::string>& a_list, const std::string a_name)
		{
			const auto it = std::find_if(
				a_list.begin(),
				a_list.end(),
				[&a_name](const std::string& a_iter)
				{
					if (a_name.size() != a_iter.size())
					{
						return false;
					}
					return std::equal(
						a_name.cbegin(),
						a_name.cend(),
						a_iter.cbegin(),
						a_iter.cend(),
						[](auto a_name, auto a_iter)
						{
							return std::toupper(a_name) == std::toupper(a_iter);
						});
				});
			return (it != a_list.end());
		}

		static bool ShouldOpenWorldMap()
		{
			if (auto Player = RE::PlayerCharacter::GetSingleton())
			{
				if (Player->GetInterior())
				{
					if (JSON::AutoInteriors)
					{
						return false;
					}
				}
				else if (auto WorldSpace = Player->GetWorldSpace())
				{
					auto editorID = WorldSpace->editorID.c_str();
					if (ListHasWorldSpace(JSON::AutoWorldSpaces, editorID))
					{
						return false;
					}

					if (JSON::AutoSmallWorld && WorldSpace->flags & 1)
					{
						return ListHasWorldSpace(JSON::AutoSmallWorldBlockList, editorID);
					}
				}
			}

			return true;
		}

		static void GetCanFastTravelFromWorldSpace()
		{
			if (auto Tile = RE::Tile::GetMenuByClass(RE::MENU_CLASS::kMapMenu))
			{
				if (auto Menu = Tile->GetMenu())
				{
					if (auto VOblivionUEPairingGate = UE::VOblivionUEPairingGate::GetSingleton())
					{
						SendHandler(
							VOblivionUEPairingGate,
							Menu->pairingEntry,
							*Main_iFrameCounter,
							"CanFastTravelFromWorldSpace",
							HandleGetCanFastTravelFromWorldSpace,
							ShouldOpenWorldMap());
					}
				}
			}
		}

		inline static REL::Hook _Hook{ REL::Offset(0x656B240), 0x46, GetCanFastTravelFromWorldSpace };

	public:
		inline static REL::Relocation<bool*> FogOfWar{ REL::Offset(0x8F9F2C0) };
	};

	static void Init()
	{
		if (JSON::DisableFogOfWar)
		{
			*hkMapMenu::FogOfWar = false;
		}
	}
}

namespace
{
	void MessageHandler(OBSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type)
		{
		case OBSE::MessagingInterface::kPostLoad:
			JSON::Init();
			HOOK::Init();
			break;
		default:
			break;
		}
	}
}

OBSE_PLUGIN_LOAD(const OBSE::LoadInterface* a_obse)
{
	OBSE::Init(a_obse, { .trampoline = true, .trampolineSize = 32 });
	OBSE::GetMessagingInterface()->RegisterListener(MessageHandler);
	return true;
}
