static void SpawnObject(string type, vector position, vector orientation)
{
    auto obj = GetGame().CreateObjectEx(type, position, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS);
    obj.SetPosition(position);
    obj.SetOrientation(orientation);
    obj.SetOrientation(obj.GetOrientation());
    obj.SetFlags(EntityFlags.STATIC, false);
    obj.Update();
	obj.SetAffectPathgraph(true, false);
	if (obj.CanAffectPathgraph()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().UpdatePathgraphRegionByObject, 100, false, obj);
}
void main()
{
	//INIT WEATHER BEFORE ECONOMY INIT------------------------
	Weather weather = g_Game.GetWeather();

	weather.MissionWeather(false);    // false = use weather controller from Weather.c

	weather.GetOvercast().Set( Math.RandomFloatInclusive(0.4, 0.6), 1, 0);
	weather.GetRain().Set( 0, 0, 1);
	weather.GetFog().Set( Math.RandomFloatInclusive(0.05, 0.1), 1, 0);

	//INIT ECONOMY--------------------------------------
	Hive ce = CreateHive();
	if ( ce )
		ce.InitOffline();

	//DATE RESET AFTER ECONOMY INIT-------------------------
	int year, month, day, hour, minute;
	int reset_month = 9, reset_day = 20;
	GetGame().GetWorld().GetDate(year, month, day, hour, minute);

	if ((month == reset_month) && (day < reset_day))
	{
		GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
	}
	else
	{
		if ((month == reset_month + 1) && (day > reset_day))
		{
			GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
		}
		else
		{
			if ((month < reset_month) || (month > reset_month + 1))
			{
				GetGame().GetWorld().SetDate(year, reset_month, reset_day, hour, minute);
			}
		}
	}

class CONFIG 
{
	// Character Creation
	const int MELEE_CHANCE 			        		= 80;		// Chance of getting a melee weapon. Out of 100. default is 100%. To Disable set to 0
	const int BACKPACK_CHANCE						= 85;		// Chance of getting a backpack. Out of 100. default is 100%. To Disable set to 0
	
	// Seasonal Settings
	const bool CHRISTMAS							= false;	// Enables christmas gear on spawn. To Disable set to false

	// Spawn Clothing Config	
	static ref const array<string> GEAR_TOPS  	   	= {"DZNC_Hoodie", "DZNC_Hoodie_Green", "DZNC_Hoodie_Grey", "DZNC_Hoodie_Blue", "DZNC_Hoodie_Red", "DZNC_Hoodie_TieDye"};
	static ref const array<string> GEAR_PANTS 		= {"CargoPants_Black", "CargoPants_Blue", "CargoPants_Green", "CargoPants_Grey"};
	static ref const array<string> GEAR_GLOVES		= {"WorkingGloves_Black", "WorkingGloves_Brown", "WorkingGloves_Beige" , "WorkingGloves_Yellow"};
	static ref const array<string> GEAR_SHOES 		= {"AthleticShoes_Black", "AthleticShoes_Green", "AthleticShoes_Blue", "AthleticShoes_Brown", "AthleticShoes_Grey"};
	static ref const array<string> GEAR_MELEE 		= {"StoneKnife", "KitchenKnife", "SteakKnife"};
	static ref const array<string> GEAR_BACKPACK 	= {"TaloonBag_Blue", "TaloonBag_Green", "TaloonBag_Orange", "TaloonBag_Violet"};
	static ref const array<string> CHEMLIGHT		= {"Chemlight_White", "Chemlight_Yellow", "Chemlight_Green", "Chemlight_Red"};
	static ref const array<string> FOOD    	    	= {"PeachesCan", "SpaghettiCan", "TacticalBaconCan", "BakedBeansCan"};
	static ref const array<string> DRINK    		= {"SodaCan_Cola", "SodaCan_Pipsi", "SodaCan_Spite", "SodaCan_Kvass"};
	static ref const array<string> MEDICAL    		= {"Bandage", "BandageDressing", "Rag"};
};

class CustomMission: MissionServer
{
	// Global Variables
	EntityAI itemTop;
	EntityAI itemEnt, Char_Bag, Char_Gloves, Char_Top, Char_Pants, Char_Shoes, Char_Chemlight, Char_Melee, attachment;
	ItemBase itemBs;

	// Convert variables to string
	string	RandomMelee;
	string	RandomBag;
	string  RandomChemlight

	// Use for randomizing items chances.
	int randNum = Math.RandomInt( 0, 100 );
	
	// Set how many bandages or rags player starts with.
	const int MEDICAL_AMOUNT = 4;	

	// Set Character Health
	void SetRandomHealth(EntityAI itemEnt)
	{
		if ( itemEnt )
		{
			float rndHlt = Math.RandomFloat( 0.75, 100 );
			itemEnt.SetHealth( "", "", rndHlt );
		}
	}

	// Set Character into game
	override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{
		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer( identity, characterName, pos, 0, "NONE" );
		Class.CastTo( m_player, playerEnt );
		GetGame().SelectPlayer( identity, m_player );
		return m_player;
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{	
		
		// Start By Removing all default items regardess of name or state
		player.RemoveAllItems(); 

		// Get Player Name
		bool playerNameIsSurvivor = false;
		string characterName = player.GetIdentity().GetName();
		characterName.ToLower();

		// Does player name contain Survivor
		if ( characterName.Contains("survivor") )
		{
			playerNameIsSurvivor = true;
		}
		
		// If Player is Named Survivor Remove All Gear
		if ( playerNameIsSurvivor )
		{
			player.RemoveAllItems();	
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SurvivorDetected, 5000, true, player);
		}
	
		// If Xmas is toggled on.  Use Christmas Clothing Set! Else use random custom hoodie, or other top
		if ( CONFIG.CHRISTMAS ) {
			player.GetInventory().CreateInInventory( "SantasHat" );
			player.GetInventory().CreateInInventory( "SantasBeard" );
            player.GetInventory().CreateInInventory( "DZNC_Xmas_Sweater");
		} else {
			Char_Top = player.GetInventory().CreateInInventory( CONFIG.GEAR_TOPS.GetRandomElement());
		}

		// Set Random Base Clothing Loadout
		Char_Pants 	  = player.GetInventory().CreateInInventory( CONFIG.GEAR_PANTS.GetRandomElement());
		Char_Shoes 	  = player.GetInventory().CreateInInventory( CONFIG.GEAR_SHOES.GetRandomElement());
		Char_Gloves   = player.GetInventory().CreateInInventory( CONFIG.GEAR_GLOVES.GetRandomElement());

		// Find player shirt slot
		itemTop = player.FindAttachmentBySlotName("Body");

		// If Shirt or Jacket is attached
		if ( itemTop )
			{
			// Give 4 Random Medical Items
			itemEnt = player.GetInventory().CreateInInventory( CONFIG.MEDICAL.GetRandomElement());
			if ( Class.CastTo(itemBs, itemEnt ) )
			itemBs.SetQuantity(MEDICAL_AMOUNT);
			// Give Random Food and Drink Items
			itemEnt = player.GetInventory().CreateInInventory( CONFIG.FOOD.GetRandomElement());
			itemEnt = player.GetInventory().CreateInInventory( CONFIG.DRINK.GetRandomElement());
		}

		// Set character Melee Weapon and add it to quickbar (Can reuse for other weapons)
		// Does the player get a Melee Weapon?
		if ( randNum <= CONFIG.MELEE_CHANCE ) {
			// Get melee weapon
			RandomMelee = CONFIG.GEAR_MELEE.GetRandomElement();
			// Create in inventory
		 	Char_Melee = player.GetInventory().CreateInInventory( RandomMelee );
			// Assign to quick bar
			player.SetQuickBarEntityShortcut( Char_Melee, 0, true );
			// Put item in hands (OPTIONAL)
			if ( GetGame().IsMultiplayer() ) {	
				player.ServerTakeEntityToHands( Char_Melee );
			} else {
				player.LocalTakeEntityToHands( Char_Melee );
			}
		}

		// Set character Backpack and attach Chemlight
		// Does the player get a Melee Weapon?
		if ( randNum <= CONFIG.BACKPACK_CHANCE ) {
			// Get Backpack and Chemlight
			// NOTE: Remember to add what ever variable use here as a string in the Global Variables
			RandomBag = CONFIG.GEAR_BACKPACK.GetRandomElement();
			RandomChemlight = CONFIG.CHEMLIGHT.GetRandomElement();
			// Create in inventory
		 	Char_Bag = player.GetInventory().CreateInInventory( RandomBag );
			// Attach Chemlight
			attachment.GetInventory().CreateAttachment( RandomChemlight );
		}		
	}
	
	// Send player message if they have Survivor as a name
	protected void SurvivorDetected(PlayerBase player)
	{
		sendPlayerMessage(player, "Since your name is 'Survivor', you spawn NAKED!");
		sendPlayerMessage(player, "If you give yourself a name, you get starter pack with clothes, backpack, basic gear!");
		sendPlayerMessage(player, "Because we are an RP Server it is highly reccomended that you pick a character name.");
	}
	
	protected void sendPlayerMessage(PlayerBase player, string message)    
    {
		if((player) && (message != ""))
		{
			Param1<string> Msgparam;
			Msgparam = new Param1<string>(message);
			GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, Msgparam, true, player.GetIdentity());
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(SurvivorDetected);
		}
    }
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}