#include <DEGLTF_Provider.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <iostream>

int main(int argc, char* argv[]) {
    const char* path = "D:/seer/3dmaster/test_models/box.glb";
    if (argc > 1) {
        path = argv[1];
    }
    std::cout << "Testing DEGLTF_Provider with: " << path << std::endl;
    occ::handle<DEGLTF_ConfigurationNode> aNode = new DEGLTF_ConfigurationNode();
    DEGLTF_Provider provider(aNode);
    TopoDS_Shape shape;
    bool ok = provider.Read(TCollection_AsciiString(path), shape);
    std::cout << "Provider Read ok: " << ok << ", isNull: " << shape.IsNull() << std::endl;
    if (ok && !shape.IsNull()) {
        int faceCount = 0;
        int totalTris = 0;
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            faceCount++;
            TopoDS_Face face = TopoDS::Face(exp.Current());
            TopLoc_Location loc;
            occ::handle<Poly_Triangulation> tri = BRep_Tool::Triangulation(face, loc);
            if (!tri.IsNull()) {
                totalTris += tri->NbTriangles();
            }
        }
        std::cout << "Face count: " << faceCount << ", Total triangles: " << totalTris << std::endl;
    }
    return ok ? 0 : 1;
}
