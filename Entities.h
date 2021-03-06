#ifndef __ENTITIES__
#define __ENTITIES__

#include "StdAfx.h"
#include "Maths.h"

class Mesh;

const int MAX_STR_LEN = 32;
typedef UINT32 ID;

enum ItemType {
	Weapon_Revolver,
	Weapon_Shotgun,
	Weapon_AK47,
	Weapon_Jackhammer,
	Weapon_Nailgun,

	Ammo_Revolver,
	Ammo_Shotgun,
	Ammo_AK47,
	Ammo_Jackhammer,
	Ammo_Nailgun,

	HealthPack,
	ArmourPack,

	Shiny,
	Monkey,
	Skull,

	ItemType_End
};

// Entry in the game database
struct Entity
{
	ID id;

	Entity(): id(0) {}

	virtual ~Entity() {}
	virtual UCHAR GetType() = 0;
	virtual int GetSize() = 0;
	virtual void OnSerializing() {};
};

// Graphical entity based on mesh
struct MeshEntity: public Entity
{
	char meshFilename[MAX_STR_LEN];
	char meshGeometryName[MAX_STR_LEN];
	Mesh* meshPtrCache;
	D3DXVECTOR3 position;
	float velocityForward;
	float velocityRight;
	float rotY;
	float rotY_multiplyByTime;
	float rotY_velocity;
	float scale;
	// Rendering options
	bool hiQuality;
	bool showOutside;
	bool showInside;

	MeshEntity() {}

	MeshEntity(string filename, string geometryName):
		meshPtrCache(NULL),
		position(D3DXVECTOR3(0, 0, 0)),
		velocityForward(0), velocityRight(0),
		rotY(0), rotY_multiplyByTime(0), rotY_velocity(0),
		scale(1.0), hiQuality(false), showOutside(true), showInside(true)
	{
		ZeroMemory(meshFilename, sizeof(meshFilename));
		ZeroMemory(meshGeometryName, sizeof(meshGeometryName));

		filename.copy(meshFilename, MAX_STR_LEN);
		geometryName.copy(meshGeometryName, MAX_STR_LEN);
	}

	static const UCHAR Type = 'M';
	UCHAR GetType() { return Type; };
	int GetSize() { return sizeof(MeshEntity); };

	Mesh* getMesh();

	void OnSerializing()
	{
		// Do not send memory address to remote computer
		meshPtrCache = NULL;
	};

	D3DXVECTOR3 ToWorldCoordinates(D3DXVECTOR3 meshVec)
	{
		return RotateY(rotY, (meshVec * scale)) + position;
	}

	D3DXVECTOR3 ToMeshCoordinates(D3DXVECTOR3 worldVec)
	{
		return RotateY(-rotY, (worldVec - position)) / scale;
	}
};

static const float PlayerMoveSpeed = 6.0f;
static const float PlayerStrafeSpeed = 3.0f;
static const float PlayerRaiseAbovePath = 0.65f;

static D3DCOLOR colorsArmour[] = {
	D3DCOLOR_XRGB(0x25, 0x2E, 0x4D),
	D3DCOLOR_XRGB(230, 80, 0),  
	D3DCOLOR_XRGB(90, 0, 90),
	D3DCOLOR_XRGB(255, 150, 0),
	D3DCOLOR_XRGB(0, 0, 0),
	D3DCOLOR_XRGB(0, 100, 0),
	D3DCOLOR_XRGB(150, 0, 0)
};
static D3DCOLOR colorsArmourDetail[] = {
	D3DCOLOR_XRGB(0x41, 0x6A, 0xAC),
	D3DCOLOR_XRGB(255, 110, 0),
	D3DCOLOR_XRGB(120, 0, 120),
	D3DCOLOR_XRGB(255, 195, 0),
	D3DCOLOR_XRGB(30, 30, 30),
	D3DCOLOR_XRGB(0, 130, 0),
	D3DCOLOR_XRGB(190, 0, 0)
};

struct Player: public MeshEntity
{
	char name[MAX_STR_LEN];
	int health;
	int armour;
	int score;
	int kills;
	int deaths;

	bool position_ServerChanged;
	ItemType selectedWeapon;
	bool selectedWeapon_ServerChanged;
	bool firing;
	double nextLoadedTime;
	int inventory[ItemType_End];

	D3DCOLOR colorArmour;
	D3DCOLOR colorArmoutDetail;

	Player() {}

	Player(string _name, int colorId):
		MeshEntity("Merman.dae", "Revolver"),
		score(0), kills(0), deaths(0),
		selectedWeapon(Weapon_Revolver), firing(false), nextLoadedTime(0),
		position_ServerChanged(false), selectedWeapon_ServerChanged(false)
	{
		ZeroMemory(name, sizeof(name));
		_name.copy(name, MAX_STR_LEN);

		int colorsCount = sizeof(colorsArmour) / sizeof(D3DCOLOR);
		colorArmour = colorsArmour[colorId % colorsCount];
		colorArmoutDetail = colorsArmourDetail[colorId % colorsCount];

		Reset();
	}

	void Reset()
	{
		health = 100;
		armour = 0;
		ZeroMemory(inventory, sizeof(inventory));
		inventory[Weapon_Revolver] = 1;
		inventory[Ammo_Revolver] = 50;
		trySelectWeapon(Weapon_Revolver);
	}

	static const UCHAR Type = 'P';
	UCHAR GetType() { return Type; };
	int GetSize() { return sizeof(Player); };

	bool trySelectWeapon(ItemType weapon)
	{
		if (inventory[weapon] == 0)
			return false;  // Do not have that weapon

		selectedWeapon = weapon;
		string geomName;
		switch(weapon) {
			case Weapon_Revolver:   geomName = inventory[Weapon_Revolver] == 2 ? "DualRevolver" : "Revolver"; break;
			case Weapon_Shotgun:    geomName = "Boomstick"; break;
			case Weapon_AK47:       geomName = "Machinegun"; break;
			case Weapon_Jackhammer: geomName = "Jackhammer"; break;
			case Weapon_Nailgun:    geomName = "Nailgun"; break;
		}
		ZeroMemory(meshGeometryName, sizeof(meshGeometryName));
		geomName.copy(meshGeometryName, MAX_STR_LEN);

		return true;
	}
};

struct Bullet: public MeshEntity
{
	ID shooter;
	D3DXVECTOR3 origin;
	float range;

	Bullet() {}
	
	Bullet(Player* _shooter, string geomName, D3DXVECTOR3 _origin, float range):
		MeshEntity("Bullets.dae", geomName),
		shooter(_shooter->id), origin(_origin), range(range)
	{
		position = _origin;
	}

	static const UCHAR Type = 'B';
	UCHAR GetType() { return Type; };
	int GetSize() { return sizeof(Bullet); };
};

struct PowerUp: public MeshEntity
{
	ItemType itemType;
	bool present;
	float rechargeAt;

	PowerUp() {}

	PowerUp(ItemType _itemType, string filename, string geometryName):
		MeshEntity(filename, geometryName),
		itemType(_itemType), present(true), rechargeAt(0)
	{
		rotY_multiplyByTime = 90;
	}

	static const UCHAR Type = 'U';
	UCHAR GetType() { return Type; };
	int GetSize() { return sizeof(PowerUp); };
};

struct RespawnPoint: public Entity
{
	D3DXVECTOR3 position;

	RespawnPoint() {}

	RespawnPoint(double x, double y, double z):
		position(D3DXVECTOR3((float)x, (float)y, (float)z))
	{}

	static const UCHAR Type = 'R';
	UCHAR GetType() { return Type; };
	int GetSize() { return sizeof(RespawnPoint); };
};

#define DbLoop(db, it) for(hash_map<ID, Entity*>::iterator it = db.begin(); it != db.end(); it++)
#define DbLoop_Meshes(db, it) } for(hash_map<ID, Entity*>::iterator it = db.begin(); it != db.end(); it++) { MeshEntity* entity = dynamic_cast<MeshEntity*>(it->second); if (entity == NULL) continue;
#define DbLoop_Players(db, it) } for(hash_map<ID, Player*>::iterator it = db.players.begin(); it != db.players.end(); it++) { Player* player = it->second;

class Database
{
	set<ID> freeIDs;
	ID nextFreeID;
	hash_map<ID, Entity*> entities;

	Database(Database& db) {} // No copying

public:

	hash_map<ID, Player*> players; // subset for optimiations

	Database(): nextFreeID(1) {}

	hash_map<ID, Entity*>::iterator begin()
	{
		return entities.begin();
	}

	hash_map<ID, Entity*>::iterator end()
	{
		return entities.end();
	}

	int size()
	{
		return entities.size();
	}

	Entity* operator[] (const ID& id)
	{
		return entities[id];
	}

	void add(Entity* entity)
	{
		if (entity->id == 0) {
			// Assign ID
			ID id;
			if (freeIDs.size() > 0) {
				id = *(freeIDs.begin());
				freeIDs.erase(freeIDs.begin());
			} else {
				id = nextFreeID++;
			}
			entity->id = id;
		}
		if (entity->id == 0xFFFFFFFF)
			throw "Readding deleted entity";

		if (entities.count(entity->id) > 0)
			throw "Consistency - ID already in dababase";

		entities.insert(pair<ID, Entity*>(entity->id, entity));
		if (typeid(*entity) == typeid(Player))
			players.insert(pair<ID, Player*>(entity->id, (Player*)entity));
	}

	void add(double x, double y, double z, float angle, MeshEntity* entity)
	{
		// Raise all power ups
		if (entity->GetType() == PowerUp::Type)
			y += 0.6;
		entity->position = D3DXVECTOR3((float)x, (float)y, (float)z);
		entity->rotY = angle;
		add(entity);
	}

	void remove(Entity* entity)
	{
		remove(entity->id);
	}

	void remove(ID id)
	{
		hash_map<ID, Entity*>::iterator it = entities.find(id);
		if (it == entities.end())
			throw "Entity not found";

		// Unassign ID
		freeIDs.insert(id);
		it->second->id = 0xFFFFFFFF;

		delete it->second;
		entities.erase(it);
		players.erase(id);
	}

	void clear()
	{
		while(entities.size() > 0) {
			remove(entities.begin()->first);
		}
	}

	void clearNonPlayers()
	{
		vector<ID> nonPlayers;
		DbLoop((*this), it) {
			if (it->second->GetType() != Player::Type)
				nonPlayers.push_back(it->first);
		}
		for(vector<ID>::iterator it = nonPlayers.begin(); it != nonPlayers.end(); it++) {
			remove(*it);
		}
	}

	~Database() { clear(); }

	void loadTestMap();
};

#endif