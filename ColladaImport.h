#include "StdAfx.h"
using namespace std;

class Mesh;

extern map<string, Mesh*> loadedMeshes;

Mesh* loadMesh(string filename, string geometryName);