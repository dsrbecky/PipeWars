#ifndef __DATABASE__
#define __DATABASE__

#include "StdAfx.h"
#include "ColladaImport.h"
using namespace std;

extern LPDIRECT3D9 pD3D;
extern LPDIRECT3DDEVICE9 pD3DDevice;

const int WeaponCount = 5;

typedef int datetime;

// Defines one or more tristrips that share same material
class Tristrip
{
	DWORD fvf;
	int vbStride; // in bytes
	vector<float> vb; // Vertex buffer data
	IDirect3DVertexBuffer9* buffer; // The buffer is created on demand
	std::vector<int> vertexCounts; // framgmentation of tristrips into groups
	string materialName;
	D3DMATERIAL9 material;
	IDirect3DTexture9* texure;

	friend Mesh* loadMesh(string filename, string geometryName);
public:
	void Render();
	void ReleaseDeviceResources() {
		if (buffer != NULL) {
			buffer->Release();
			buffer = NULL;
		}
	}
};

class Mesh
{
public:
	std::vector<Tristrip> tristrips;

	void Render();
	void ReleaseDeviceResources() {
		for(int i = 0; i < (int)tristrips.size(); i++) {
			tristrips[i].ReleaseDeviceResources();
		}
	}
};

class TextWriter
{
	ID3DXFont* font;
public:
	TextWriter(): font(NULL) {}
	void Render();
	void ReleaseDeviceResources() {
		if (font != NULL) {
			font->Release();
			font = NULL;
		}
	}
};

class Vec3
{
public:
	float x, y, z;

	Vec3(float _x, float _y, float _z): x(_x), y(_y), z(_z) {}

	static Vec3 Zero() { return Vec3(0, 0, 0); }
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
	Vec3 position;
	Vec3 velocity;
	float rotY;
	float rotY_velocity;
	float scale;

	MeshEntity(string filename, string geometryName):
		mesh(loadMesh(filename, geometryName)),
		position(Vec3::Zero()), velocity(Vec3::Zero()),
		rotY(0), rotY_velocity(0), scale(1) {}

	void Render() { mesh->Render(); };
};

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
	datetime rechargeAt;

	PowerUp(ItemType _itemType): MeshEntity("Shiny.dae", "Green"), itemType(_itemType), present(true), rechargeAt(0) {}
};

class RespawnPoint: public Entity
{
public:
	Vec3 position;

	RespawnPoint(double x, double y, double z): position(Vec3((float)x, (float)y, (float)z)) {}
};

class ChatMessage: public Entity
{
public:
	datetime time;
	Player* sender;
	string message;

	ChatMessage(datetime _time, Player* _sender, string _message):
		time(_time), sender(_sender), message(_message) {}
};

class Database
{
public:
	vector<Entity*> entities;

	void Render();

	void add(Entity* entity)
	{
		entities.push_back(entity);
	}

	void add(double x, double y, double z, float angle, MeshEntity* entity)
	{
		entity->position = Vec3((float)x, (float)y, (float)z);
		entity->rotY = angle;
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
