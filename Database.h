#ifndef __DATABASE__
#define __DATABASE__

#include "StdAfx.h"
#include <set>
#include <list>

const int WeaponCount = 6;
const float NearClip = 2.0f;
const float FarClip = 1000.0f;
static const string PipeFilename = "pipe.dae";

class Mesh; extern Mesh* loadMesh(string filename, string geometryName); // ColladaImport

bool IsPointOnPath(D3DXVECTOR3 pos, float* outY = NULL);

inline D3DXVECTOR3 RotYToDirecion(float rotY_Deg)
{
	float rotY_Rad = rotY_Deg / 360 * 2 * D3DX_PI;
	return D3DXVECTOR3(-sin(rotY_Rad), 0, cos(rotY_Rad));
}

// Defines one or more tristrips that share same material
class Tristrip
{
	DWORD fvf;
	int vbStride_bytes;
	int vbStride_floats;
	vector<float> vb; // Vertex buffer data
	IDirect3DVertexBuffer9* buffer; // The buffer is created on demand
	std::vector<int> vertexCounts; // framgmentation of tristrips into groups
	string materialName;
	D3DMATERIAL9 material;
	string textureFilename;

	friend Mesh* loadMesh(string filename, string geometryName);
public:
	string getMaterialName() { return materialName; }

	void Render(IDirect3DDevice9* dev);
	void ReleaseDeviceResources() {
		if (buffer != NULL) {
			buffer->Release();
			buffer = NULL;
		}
	}

	bool IntersectsYRay(float x, float z, float* outY = NULL);
};

class BoundingBox
{
public:
	D3DXVECTOR3 minCorner;
	D3DXVECTOR3 maxCorner;
	D3DXVECTOR3 corners[8];

	BoundingBox(D3DXVECTOR3 _min, D3DXVECTOR3 _max): minCorner(_min), maxCorner(_max) {
		corners[0] = D3DXVECTOR3(minCorner.x, minCorner.y, minCorner.z);
		corners[1] = D3DXVECTOR3(minCorner.x, minCorner.y, maxCorner.z);
		corners[2] = D3DXVECTOR3(minCorner.x, maxCorner.y, minCorner.z);
		corners[3] = D3DXVECTOR3(minCorner.x, maxCorner.y, maxCorner.z);
		corners[4] = D3DXVECTOR3(maxCorner.x, minCorner.y, minCorner.z);
		corners[5] = D3DXVECTOR3(maxCorner.x, minCorner.y, maxCorner.z);
		corners[6] = D3DXVECTOR3(maxCorner.x, maxCorner.y, minCorner.z);
		corners[7] = D3DXVECTOR3(maxCorner.x, maxCorner.y, maxCorner.z);
	}

	bool Contains(D3DXVECTOR3 vec)
	{
		return
			minCorner.x <= vec.x && vec.x <= maxCorner.x &&
			minCorner.y <= vec.y && vec.y <= maxCorner.y &&
			minCorner.z <= vec.z && vec.z <= maxCorner.z;
	}
};

static string pathMaterialName = "Path";

class Mesh
{
public:
	string filename;
	string geometryName;
	BoundingBox boundingBox;
	std::vector<Tristrip> tristrips;

	Mesh(): boundingBox(BoundingBox(D3DXVECTOR3(0,0,0), D3DXVECTOR3(0,0,0))) {}
	void Render(IDirect3DDevice9* dev, string hide1 = "", string hide2 = "", string hide3 = "");
	void ReleaseDeviceResources() {
		for(int i = 0; i < (int)tristrips.size(); i++) {
			tristrips[i].ReleaseDeviceResources();
		}
	}

	bool IsOnPath(float x, float z, float* outY = NULL);
};

enum ItemType {
	Weapon_Revolver,
	Weapon_DualRevolver,
	Weapon_Shotgun,
	Weapon_AK47,
	Weapon_Jackhammer,
	Weapon_Nailgun,

	Ammo_Revolver,
	Ammo_DualRevolver,
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
class Entity
{
public:
	int id;

	Entity(): id(0) {}

	virtual ~Entity() {}
};

// Graphical entity based on mesh
class MeshEntity: public Entity
{
public:
	Mesh* mesh;
	D3DXVECTOR3 position;
	D3DXVECTOR3 velocity;
	float rotY;
	float rotY_velocity;
	float scale;

	bool hiQuality;

	MeshEntity(string filename, string geometryName):
		mesh(loadMesh(filename, geometryName)),
		position(D3DXVECTOR3(0,0,0)), velocity(D3DXVECTOR3(0,0,0)),
		rotY(0), rotY_velocity(0), scale(1), hiQuality(true) {}
};

static const float PlayerMoveSpeed = 5.0f;
static const float PlayerStrafeSpeed = 3.0f;
static const float PlayerRaiseAbovePath = 0.65f;

class Player: public MeshEntity
{
public:

	string name;
	int health;
	int armour;
	int score;
	int kills;
	int deaths;

	ItemType selectedWeapon;
	int inventory[ItemType_End];

	Player(string _name):
		MeshEntity("Merman.dae", "Revolver"),
		name(_name), health(100), armour(0), score(0), kills(0), deaths(0), selectedWeapon(Weapon_Revolver)
	{
		ZeroMemory(&inventory, sizeof(inventory));
		inventory[Weapon_Revolver] = 1;
		inventory[Ammo_Revolver] = 999;
		inventory[Ammo_DualRevolver] = 999;
	}

	bool selectWeapon(ItemType weapon)
	{
		if (inventory[weapon] == 0)
			return false;  // Do not have that weapon

		selectedWeapon = weapon;
		switch(weapon) {
			case Weapon_Revolver:
				mesh = loadMesh("Merman.dae", "Revolver");
				break;
			case Weapon_DualRevolver:
				mesh = loadMesh("Merman.dae", "DualRevolver");
				break;
			case Weapon_Shotgun:
				mesh = loadMesh("Merman.dae", "Boomstick");
				break;
			case Weapon_AK47:
				mesh = loadMesh("Merman.dae", "Machinegun");
				break;
			case Weapon_Jackhammer:
				mesh = loadMesh("Merman.dae", "Jackhammer");
				break;
			case Weapon_Nailgun:
				mesh = loadMesh("Merman.dae", "Nailgun");
				break;
		}

		return true;
	}
};

class Bullet: public MeshEntity
{
public:
	Player* shooter;
	int weapon;
	float rangeLeft;
	
	Bullet(Player* _shooter, int _weapon):
		MeshEntity("Bullets.dae", "RevolverBullet"), shooter(_shooter), weapon(_weapon), rangeLeft(10) {}
};

class PowerUp: public MeshEntity
{
public:
	ItemType itemType;
	bool present;
	float rechargeAfter;

	PowerUp(ItemType _itemType, string filename, string geometryName):
		MeshEntity(filename, geometryName),
		itemType(_itemType), present(true), rechargeAfter(0) {
		rotY_velocity = 90;
	}
};

class RespawnPoint: public Entity
{
public:
	D3DXVECTOR3 position;

	RespawnPoint(double x, double y, double z): position(D3DXVECTOR3((float)x, (float)y, (float)z)) {}
};

class ChatMessage: public Entity
{
public:
	float time;
	Player* sender;
	string message;

	ChatMessage(float _time, Player* _sender, string _message):
		time(_time), sender(_sender), message(_message) {}
};

class Database
{
public:
	list<Entity*> entities;

	void add(Entity* entity)
	{
		entities.push_back(entity);
	}

	void add(double x, double y, double z, float angle, MeshEntity* entity)
	{
		entity->position = D3DXVECTOR3((float)x, (float)y, (float)z);
		entity->rotY = angle;
		entities.push_back(entity);
	}

	void loadTestMap();

	void clear() {
		for(list<Entity*>::iterator it = entities.begin(); it != entities.end(); it++) {
			delete *it;
		}
		entities.clear();
	}

	~Database() { clear(); }
};

static inline D3DXVECTOR3 min3(D3DXVECTOR3 a, D3DXVECTOR3 b)
{
	return D3DXVECTOR3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

static inline D3DXVECTOR3 max3(D3DXVECTOR3 a, D3DXVECTOR3 b)
{
	return D3DXVECTOR3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}

extern Database db;

#endif __DATABASE__
