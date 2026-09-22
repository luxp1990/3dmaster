#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <windows.h>

#include <STEPControl_Reader.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <BRepLib.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp.hxx>
#include <Geom2d_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepLProp_SLProps.hxx>

#include <STEPCAFControl_Reader.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDocStd_Document.hxx>
#include <Quantity_Color.hxx>
#include <TDF_LabelSequence.hxx>

int wmain(int argc, wchar_t* argv[]) {
    std::filesystem::path p(L"C:/Users/50350/Downloads/三腔接头-260416-2.stp");
    if (argc > 1) {
        p = argv[1];
    }

    std::wcout << L"Testing OCCT STEP loading & tessellation: " << p.wstring() << std::endl;
    auto t0 = std::chrono::high_resolution_clock::now();
    std::ifstream stream(p, std::ios::in | std::ios::binary);
    if (!stream.is_open()) {
        std::wcerr << L"Failed to open file stream: " << p.wstring() << std::endl;
        return 1;
    }

    occ::handle<TDocStd_Document> doc;
    XCAFApp_Application::GetApplication()->NewDocument("MDTV-XCAF", doc);

    STEPCAFControl_Reader reader;
    reader.SetColorMode(true);
    reader.SetNameMode(true);
    reader.SetLayerMode(true);
    IFSelect_ReturnStatus status = reader.ReadStream(p.filename().string().c_str(), stream);
    if (status != IFSelect_RetDone) {
        std::cerr << "Failed to read STEP file from stream, status = " << status << std::endl;
        return 1;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "STEP ReadStream: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
              << " ms" << std::endl;

    reader.Transfer(doc);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "STEP Transfer to XCAF Doc: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
              << " ms" << std::endl;

    occ::handle<XCAFDoc_ShapeTool> shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    occ::handle<XCAFDoc_ColorTool> colorTool = XCAFDoc_DocumentTool::ColorTool(doc->Main());

    TopoDS_Shape shape = shapeTool->GetOneShape();
    if (shape.IsNull()) {
        std::cerr << "shapeTool->GetOneShape() is NULL" << std::endl;
        return 1;
    }

    TDF_LabelSequence freeShapes;
    shapeTool->GetFreeShapes(freeShapes);
    std::cout << "freeShapes.Length(): " << freeShapes.Length() << std::endl;
    int solidCount = 0;
    for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
        solidCount++;
    }
    std::cout << "solidCount: " << solidCount << std::endl;
    for (int i = 1; i <= freeShapes.Length(); ++i) {
        TDF_Label l = freeShapes.Value(i);
        Quantity_Color partCol;
        if (colorTool->GetColor(l, XCAFDoc_ColorGen, partCol) ||
            colorTool->GetColor(l, XCAFDoc_ColorSurf, partCol)) {
            std::cout << "Part " << i << " has Color: R=" << partCol.Red() << " G=" << partCol.Green() << " B=" << partCol.Blue() << std::endl;
        }
    }

    auto t1_5 = std::chrono::high_resolution_clock::now();
    Bnd_Box bndBox;
    BRepBndLib::AddClose(shape, bndBox);
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bndBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;
    double diag = std::sqrt(dx * dx + dy * dy + dz * dz);
    auto t1_6 = std::chrono::high_resolution_clock::now();
    std::cout << "BRepBndLib::AddClose took: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(t1_6 - t1_5).count() 
              << " ms, diag=" << diag << std::endl;

    // 自适应公差：对角线的 1/1000，限制在 [0.01mm, 1.5mm]
    double linearDeflection = std::clamp(diag * 0.001, 0.01, 1.5);
    double angularDeflection = 0.5;
    std::cout << "Using adaptive linearDeflection = " << linearDeflection << std::endl;
    BRepMesh_IncrementalMesh mesher(shape, linearDeflection, false, angularDeflection, true);
    auto t2_5 = std::chrono::high_resolution_clock::now();
    bool corrected = BRepLib::EnsureNormalConsistency(shape, 0.001, true);
    auto t3 = std::chrono::high_resolution_clock::now();
    std::cout << "EnsureNormalConsistency took: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2_5).count()
              << " ms, corrected=" << corrected << std::endl;

    uint64_t totalNodes = 0;
    uint64_t totalTriangles = 0;
    int faceCount = 0;

    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9, minZ = 1e9, maxZ = -1e9;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        occ::handle<Poly_Triangulation> tri = BRep_Tool::Triangulation(face, loc);
        if (!tri.IsNull()) {
            if (faceCount == 0) {
                std::cout << "tri->HasNormals() on first face: " << tri->HasNormals() << std::endl;
            }
            faceCount++;
            Quantity_Color col;
            if (colorTool->GetColor(face, XCAFDoc_ColorSurf, col)) {
                static int redFaces = 0;
                redFaces++;
                if (redFaces <= 5) {
                    std::cout << "Face " << faceCount << " has ColorSurf: R=" << col.Red() << " G=" << col.Green() << " B=" << col.Blue() << std::endl;
                }
            } else if (colorTool->GetColor(face, XCAFDoc_ColorGen, col)) {
                static int genFaces = 0;
                genFaces++;
                if (genFaces <= 3) {
                    std::cout << "Face " << faceCount << " has ColorGen: R=" << col.Red() << " G=" << col.Green() << " B=" << col.Blue() << std::endl;
                }
            }
            totalNodes += tri->NbNodes();
            totalTriangles += tri->NbTriangles();
            gp_Trsf trsf = loc.Transformation();
            bool hasTrsf = !loc.IsIdentity();
            for (int i = 1; i <= tri->NbNodes(); ++i) {
                gp_Pnt p = tri->Node(i);
                if (hasTrsf) p.Transform(trsf);
                minX = std::min(minX, p.X()); maxX = std::max(maxX, p.X());
                minY = std::min(minY, p.Y()); maxY = std::max(maxY, p.Y());
                minZ = std::min(minZ, p.Z()); maxZ = std::max(maxZ, p.Z());

                if (tri->HasNormals()) {
                    try {
                        gp_Dir normDir = tri->Normal(i);
                    } catch (const Standard_Failure& e) {
                        std::cerr << "CAUGHT in tri->Normal(" << i << ") on face " << faceCount << ": " << e.GetMessageString() << std::endl;
                    }
                }
            }
        }
    }

    std::cout << "Testing Edges dihedral check..." << std::endl;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFaces;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeToFaces);
    for (int eIdx = 1; eIdx <= edgeToFaces.Extent(); ++eIdx) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeToFaces.FindKey(eIdx));
        const TopTools_ListOfShape& faceList = edgeToFaces.FindFromIndex(eIdx);
        if (faceList.Extent() == 2) {
            TopoDS_Face f1 = TopoDS::Face(faceList.First());
            TopoDS_Face f2 = TopoDS::Face(faceList.Last());

            double firstParam = 0.0, lastParam = 0.0;
            occ::handle<Geom2d_Curve> c2d1 = BRep_Tool::CurveOnSurface(edge, f1, firstParam, lastParam);
            occ::handle<Geom2d_Curve> c2d2 = BRep_Tool::CurveOnSurface(edge, f2, firstParam, lastParam);
            if (!c2d1.IsNull() && !c2d2.IsNull()) {
                double midParam = (firstParam + lastParam) * 0.5;
                gp_Pnt2d uv1 = c2d1->Value(midParam);
                gp_Pnt2d uv2 = c2d2->Value(midParam);

                BRepAdaptor_Surface s1(f1, false);
                BRepAdaptor_Surface s2(f2, false);

                BRepLProp_SLProps props1(s1, uv1.X(), uv1.Y(), 1, 1e-4);
                BRepLProp_SLProps props2(s2, uv2.X(), uv2.Y(), 1, 1e-4);

                try {
                    if (props1.IsNormalDefined()) {
                        gp_Dir n1 = props1.Normal();
                    }
                } catch (const Standard_Failure& e) {
                    std::cerr << "CAUGHT in props1.Normal() on edge " << eIdx << ": " << e.GetMessageString() << std::endl;
                }
                try {
                    if (props2.IsNormalDefined()) {
                        gp_Dir n2 = props2.Normal();
                    }
                } catch (const Standard_Failure& e) {
                    std::cerr << "CAUGHT in props2.Normal() on edge " << eIdx << ": " << e.GetMessageString() << std::endl;
                }
            }
        }
    }

    std::cout << "Bounds X: [" << minX << ", " << maxX << "] (size=" << (maxX - minX) << ")\n";
    std::cout << "Bounds Y: [" << minY << ", " << maxY << "] (size=" << (maxY - minY) << ")\n";
    std::cout << "Bounds Z: [" << minZ << ", " << maxZ << "] (size=" << (maxZ - minZ) << ")\n";
    std::cout << "Center: (" << (minX+maxX)*0.5 << ", " << (minY+maxY)*0.5 << ", " << (minZ+maxZ)*0.5 << ")\n";

    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << "Extract Triangles: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()
              << " ms" << std::endl;
    std::cout << "TOTAL TIME: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t0).count()
              << " ms" << std::endl;
    std::cout << "Faces with mesh: " << faceCount
              << ", Total vertices: " << totalNodes
              << ", Total triangles: " << totalTriangles
              << std::endl;

    return 0;
}
