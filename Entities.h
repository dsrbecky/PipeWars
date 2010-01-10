#ifndef __ENTITIES__
#define __ENTITIES__

#include "StdAfx.h"

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

	MeshEntity() {}

	MeshEntity(string filename, string geometryName):
		meshPtrCache(NULL),
		position(D3DXVECTOR3(0, 0, 0)),
		velocityForward(0), velocityRight(0),
		rotY(0), rotY_multiplyByTime(0), rotY_velocity(0),
		scale(1.0), hiQuality(false)
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
};

static const float PlayerMoveSpeed = 6.0f;
static const float PlayerStrafeSpeed = 3.0f;
static const float PlayerRaiseAbovePath = 0.65f;

struct Player: public MeshEntity
{
	char name[MAX_STR_LEN];
	int health;
	int armour;
	int score;
	int kills;
	int deaths;

	ItemType selectedWeapon;
	bool firing;
	double nextLoadedTime;
	int inventory[ItemType_End];

	Player() {}

	Player(string _name):
		MeshEntity("Merman.dae", "Revolver"),
		health(100), armour(0),
		score(0), kills(0), deaths(0),
		selectedWeapon(Weapon_Revolver), firing(false), nextLoadedTime(0)
	{
		ZeroMemory(name, sizeof(name));
		ZeroMemory(inventory, sizeof(inventory));

		_name.copy(name, MAX_STR_LEN);
		inventory[Weapon_Revolver] = 1;
		inventory[Ammo_Revolver] = 48;
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
	int weapon;
	D3DXVECTOR3 origin;
	float range;

	Bullet() {}
	
	Bullet(Player* _shooter, int _weapon, D3DXVECTOR3 _position, float range):
		MeshEntity("Bullets.dae", "RevolverBullet"),
		shooter(_shooter->id), weapon(_weapon),
		origin(_position), range(range)
	{
		position = _position;
	}

	static const UCHAR Type = 'B';
	UCHAR GetType() { return Type; };
	int GetSize() { return sizeof(Bullet); };
};

struct PowerUp: public MeshEntity
{
	ItemType itemType;
	bool present;
	float rechargeAfter;

	PowerUp() {}

	PowerUp(ItemType _itemType, string filename, string geometryName):
		MeshEntity(filename, geometryName),
		itemType(_itemType), present(true), rechargeAfter(0)
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

#define DbLoop(it) for(hash_map<ID, Entity*>::iterator it = db.begin(); it != db.end(); it++)

class Database
{
	set<ID> freeIDs;
	ID nextFreeID;
	hash_map<ID, Entity*> entities;

public:

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
		if (entity->id != 0)
			throw "Entity already in database?";

		// Assign ID
		ID id;
		if (freeIDs.size() > 0) {
			id = *(freeIDs.begin());
			freeIDs.erase(freeIDs.begin());
		} else {
			id = nextFreeID++;
		}
		entity->id = id;

		if (entities.count(id) > 0)
			throw "Consistency - ID already in dababase";

		entities.insert(pair<ID, Entity*>(id, entity));
	}

	void add(double x, double y, double z, float angle, MeshEntity* entity)
	{
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
	}

	void clear()
	{
		while(entities.size() > 0) {
			remove(entities.begin()->first);
		}
	}

	~Database() { clear(); }

	void loadTestMap();
};

#endif