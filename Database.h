#ifndef __DATABASE__
#define __DATABASE__

#include "StdAfx.h"

class Mesh; extern Mesh* loadMesh(string filename, string geometryName); // ColladaImport
const int WeaponCount = 5;

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
	IDirect3DTexture9* texture;

	friend Mesh* loadMesh(string filename, string geometryName);
public:
	string getMaterialName() { return materialName; }

	void Render(IDirect3DDevice9* dev);
	void ReleaseDeviceResources() {
		if (buffer != NULL) {
			buffer->Release();
			buffer = NULL;
		}
		if (texture != NULL) {
			texture->Release();
			texture = NULL;
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
};

class Mesh
{
public:
	BoundingBox boundingBox;
	std::vector<Tristrip> tristrips;

	Mesh(): boundingBox(BoundingBox(D3DXVECTOR3(0,0,0), D3DXVECTOR3(0,0,0))) {}
	void Render(IDirect3DDevice9* dev, string hide1 = "", string hide2 = "");
	void ReleaseDeviceResources() {
		for(int i = 0; i < (int)tristrips.size(); i++) {
			tristrips[i].ReleaseDeviceResources();
		}
	}
};

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
	Monkey
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

	MeshEntity(string filename, string geometryName):
		mesh(loadMesh(filename, geometryName)),
		position(D3DXVECTOR3(0,0,0)), velocity(D3DXVECTOR3(0,0,0)),
		rotY(0), rotY_velocity(0), scale(1) {}
};

static const float PlayerMoveSpeed = 5.0f;
static const float PlayerStrafeSpeed = 3.0f;

class Player: public MeshEntity
{
public:

	string name;
	int health;
	int armour;
	int score;
	int kills;
	int deaths;

	int selectedWeapon;
	int inventory[sizeof(ItemType)];

	Player(string _name):
		MeshEntity("Merman.dae", "Revolver"),
		name(_name), health(100), armour(0), score(0), kills(0), deaths(0), selectedWeapon(Weapon_Revolver)
	{
		ZeroMemory(&inventory, sizeof(inventory));
		inventory[Weapon_Revolver] = 1;
	}
};

class Pipe: public MeshEntity
{
public:
	Pipe(string geometryName): MeshEntity("pipe.dae", geometryName) {}
};

class Tank: public MeshEntity
{
public:
	Tank(string geometryName): MeshEntity("tank.dae", geometryName) {}
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

	PowerUp(ItemType _itemType): MeshEntity("Shiny.dae", "Green"), itemType(_itemType), present(true), rechargeAfter(0) {}
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
	vector<Entity*> entities;

	void add(Entity* entity)
	{
		entities.push_back(entity);
	}

	void add(double x, double y, double z, float angle, MeshEntity* entity)
	{
		entity->position = D3DXVECTOR3((float)x, (float)y, (float)z);
		entity->rotY = -angle;
		entities.push_back(entity);
	}

	void loadTestMap();

	void clear() {
		for(int i = 0; i < (int)entities.size(); i++) {
			delete entities[i];
		}
		entities.clear();
	}

	~Database() { clear(); }
};

extern Database db;

#endif __DATABASE__
