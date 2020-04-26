
= 雰囲気で始めるマーチングキューブス法入門

== マーチングキューブス法とは？

=== 歴史と概要


マーチングキューブス法とは、ボリュームレンダリング法の一つで、スカラーデータで満たされた3次元ボクセルデータを、ポリゴンデータに変換するアルゴリズムです。William E. Lorensen と Harvey E. Cline によって1987年に最初の論文が発表されました。

マーチングキューブス法は特許が取得されていましたが、2005年に特許が切れているので、現在は自由に使用できます。


=== 簡単な仕組みの解説


まず、ボリュームデータの空間を3次元グリッドで分割します。
//image[marching_cubes_001@4x][3次元ボリュームデータとグリッド分割]{
//}




次に分割したグリッドの1つを取り出してみましょう。グリッドの８つの角の値が閾値以上だったら 1 、閾値未満だったら 0 として、８頂点の境界を割り出します。@<br>{}
以下の図は、閾値を0.5とした場合の流れです。
//image[marching_cubes_002@4x][角の値に応じて境界を割り出す]{
//}




その8つの角の組み合わせは256種類ありますが、回転や反転を駆使すると15種類に収まります。その15種類の組み合わせに対応した三角ポリゴンのパターンを割り当てます。
//image[marching_cubes_003@4x][角の組み合わせ]{
//}



== サンプルリポジトリ

本章で解説するサンプルプロジェクトは、UnityGraphicsProgrammingのUnityプロジェクトhttps://github.com/IndieVisualLab/UnityGraphicsProgramming内にあるAssets/GPUMarchingCubesにあります。


実装にあたり、Paul Bourke氏のPolygonising a scalar fieldのサイト@<fn>{paul}を参考に、Unityに移植させて頂きました。  
//footnote[paul][Polygonising a scalar field http://paulbourke.net/geometry/polygonise/]


今回はこのサンプルプロジェクトに沿って解説していきます。  



実装は大きくわけて3つあります。  

 * メッシュの初期化、毎フレームの描画登録処理（C#スクリプト部分）
 * ComputeBufferの初期化
 * 実際の描画処理（シェーダー部分）



まずは、メッシュの初期化や描画登録をする @<b>{GPUMarchingCubesDrawMesh} クラスから作っていきます。


=== GeometryShader用のメッシュを作る


前項で説明したとおり、マーチングキューブス法はグリッドの8つの角の組み合わせでポリゴンを生成するアルゴリズムです。
リアルタイムにそれを行うには、動的にポリゴンを作る必要があります。@<br>{}
しかし、毎フレームCPU側(C#側)でメッシュの頂点配列を生成するのは非効率です。@<br>{}
そこで、GeometryShaderを使います。GeometryShaderは、大雑把に説明するとVertexShaderとFragmentShaderの間に位置するShaderで、VertexShaderで処理された頂点を増減させることができます。@<br>{}
例えば、1頂点の周囲に6つの頂点を追加して板ポリゴンを生成したりできます。@<br>{}
更に、Shader側(GPU側)で処理するのでとても高速です。@<br>{}
今回はGeometryShaderを使ってMarchingCubesのポリゴンを生成して表示してみます。  



まず、 @<b>{GPUMarchingCubesDrawMesh}クラスで使う変数群を定義します。


//list[kaiware_define_cs][変数群の定義部分][cs]{
using UnityEngine;

public class GPUMarchingCubesDrawMesh : MonoBehaviour {

    #region public
    public int segmentNum = 32;                 // グリッドの一辺の分割数

    [Range(0,1)]
    public float threashold = 0.5f;             // メッシュ化するスカラー値のしきい値
    public Material mat;                        // レンダリング用のマテリアル

    public Color DiffuseColor = Color.green;    // ディフューズカラー
    public Color EmissionColor = Color.black;   // 発光色
    public float EmissionIntensity = 0;         // 発光の強さ

    [Range(0,1)]
    public float metallic = 0;                  // メタリック感
    [Range(0, 1)]
    public float glossiness = 0.5f;             // 光沢感
    #endregion

     #region private
    int vertexMax = 0;                          // 頂点数
    Mesh[] meshs = null;                        // Mesh配列
    Material[] materials = null;                // Meshごとのマテリアル配列
    float renderScale = 1f / 32f;               // 表示スケール
    MarchingCubesDefines mcDefines = null;      // MarchingCubes用定数配列群
    #endregion

}
//}


次にGeometryShaderに渡すためのメッシュを作成します。メッシュの頂点は、分割した3次元グリッド内に1個ずつ配置するようにします。
例えば、一辺の分割数が64の場合、64*64*64=262,144個もの頂点が必要になります。

しかし、Unity2017.1.1f1において、１つのメッシュの頂点数は65,535個が上限となってます。
その為、メッシュ１つにつき、頂点数を65,535個以内に収める形で分割します。  


//list[kaiware_create_mesh_cs][メッシュ作成部分][cs]{
void Initialize()
{
  vertexMax = segmentNum * segmentNum * segmentNum;

  Debug.Log("VertexMax " + vertexMax);

  // 1Cubeの大きさをsegmentNumで分割してレンダリング時の大きさを決める
  renderScale = 1f / segmentNum;

  CreateMesh();

  // シェーダーで使うMarchingCubes用の定数配列の初期化
  mcDefines = new MarchingCubesDefines();
}

void CreateMesh()
{
  // Meshの頂点数は65535が上限なので、Meshを分割する
  int vertNum = 65535;
  int meshNum = Mathf.CeilToInt((float)vertexMax / vertNum);  // 分割するMeshの数
  Debug.Log("meshNum " + meshNum );

  meshs = new Mesh[meshNum];
  materials = new Material[meshNum];

  // Meshのバウンズ計算
  Bounds bounds = new Bounds(
    transform.position, 
    new Vector3(segmentNum, segmentNum, segmentNum) * renderScale
  );

  int id = 0;
  for (int i = 0; i < meshNum; i++)
  {
    // 頂点作成
    Vector3[] vertices = new Vector3[vertNum];
    int[] indices = new int[vertNum];
    for(int j = 0; j < vertNum; j++)
    {
      vertices[j].x = id % segmentNum;
      vertices[j].y = (id / segmentNum) % segmentNum;
      vertices[j].z = (id / (segmentNum * segmentNum)) % segmentNum;

      indices[j] = j;
      id++;
    }

    // Mesh作成
    meshs[i] = new Mesh();
    meshs[i].vertices = vertices;
    // GeometryShaderでポリゴンを作るのでMeshTopologyはPointsで良い
    meshs[i].SetIndices(indices, MeshTopology.Points, 0);
    meshs[i].bounds = bounds;

    materials[i] = new Material(mat);
  }
}
//}




=== ComputeBufferの初期化


@<b>{MarchingCubesDefinces.cs} というソースには、マーチングキューブス法のレンダリングで使う定数配列と、その定数配列をシェーダーに渡すためのComputeBufferが定義されています。
ComputeBufferとは、シェーダーで使うデータを格納するバッファです。データはGPU側のメモリに置かれるのでシェーダーからのアクセスが早いです。

実は、マーチングキューブス法のレンダリングで使う定数配列は、シェーダー側で定義することは可能です。
しかし、何故シェーダーで使う定数配列を、C#側で初期化しているのかというと、シェーダーにはリテラル値(直書きした値)の個数が4096までしか登録出来ない制限があるためです。膨大な定数配列をシェーダー内に定義すると、あっという間にリテラル値の数の上限に到達してしまいます。

そこで、ComputeShaderに格納して渡すことで、リテラル値ではなくなるので上限にひっかからなくなります。
そのため、工程が少々増えてしまいますが、C#側でComputeBufferに定数配列を格納してシェーダーに渡すようにしています。


//list[kaiware_computebuffer_init][ComputeBufferの初期化部分][cs]{
void Initialize()
{
  vertexMax = segmentNum * segmentNum * segmentNum;

  Debug.Log("VertexMax " + vertexMax);

  // 1Cubeの大きさをsegmentNumで分割してレンダリング時の大きさを決める
  renderScale = 1f / segmentNum;

  CreateMesh();

  // シェーダーで使うMarchingCubes用の定数配列の初期化
  mcDefines = new MarchingCubesDefines();
}
//}


先程のInitialize()関数の中で、MarchingCubesDefinesの初期化を行っています。


=== レンダリング


次にレンダリング処理を呼び出す関数です。@<br>{}
今回は、複数のメッシュを一度にレンダリングするのと、Unityのライティングの影響を受けられるようにするため、Graphics.DrawMesh() を使います。public 変数で定義したDiffuseColor等の意味は、シェーダー側の解説で説明します。

前項の、MarchingCubesDefinesクラスのComputeBuffer達をmaterial.setBufferでシェーダーに渡しています。


//list[kaiware_rendermesh][レンダリング部分][cs]{
void RenderMesh()
{
  Vector3 halfSize = new Vector3(segmentNum, segmentNum, segmentNum) 
                     * renderScale * 0.5f;
  Matrix4x4 trs = Matrix4x4.TRS(
                     transform.position, 
                     transform.rotation, 
                     transform.localScale
                  );
        
  for (int i = 0; i < meshs.Length; i++)
  {
    materials[i].SetPass(0);
    materials[i].SetInt("_SegmentNum", segmentNum);
    materials[i].SetFloat("_Scale", renderScale);
    materials[i].SetFloat("_Threashold", threashold);
    materials[i].SetFloat("_Metallic", metallic);
    materials[i].SetFloat("_Glossiness", glossiness);
    materials[i].SetFloat("_EmissionIntensity", EmissionIntensity);

    materials[i].SetVector("_HalfSize", halfSize);
    materials[i].SetColor("_DiffuseColor", DiffuseColor);
    materials[i].SetColor("_EmissionColor", EmissionColor);
    materials[i].SetMatrix("_Matrix", trs);

    Graphics.DrawMesh(meshs[i], Matrix4x4.identity, materials[i], 0);
  }
}
//}

== 呼び出し

//list[kaiware_update][呼び出し部分][cs]{
// Use this for initialization
void Start () 
{
  Initialize();
}

void Update()
{
  RenderMesh();
}
//}


Start()でInitialize()を呼び出してメッシュを生成、Update()関数でRenderMesh()を呼び出してレンダリングします。@<br>{}
Update()でRenderMesh()を呼び出す理由は、Graphics.DrawMesh()が即座に描画するわけではなく、「レンダリング処理に一旦登録する」という感じのものだからです。@<br>{}
登録することで、Unityがライトやシャドウを適応してくれます。似たような関数にGraphics.DrawMeshNow()がありますが、こちらは即座に描画するのでUnityのライトやシャドウが適応されません。また、Update()ではなく、OnRenderObject()やOnPostRender()などで呼び出す必要があります。


== シェーダ側の実装


今回のシェーダは、大きく分けて@<b>{「実体のレンダリング部」}と@<b>{「影のレンダリング部」}の２つに分かれます。
さらに、それぞれの中で、頂点シェーダ、ジオメトリシェーダ、フラグメントシェーダの3つのシェーダ関数が実行されます。

シェーダーのソースが長いので、実装全体はサンプルプロジェクトの方を見てもらうことにして、要所要所だけ解説します。
解説するシェーダーのファイルは、GPUMarchingCubesRenderMesh.shaderです。


=== 変数の宣言


シェーダーの上の方では、レンダリングで使う構造体の定義をしています。


//list[kaiware_shader_struct_define][構造体の定義部分][cs]{
// メッシュから渡ってくる頂点データ
struct appdata
{
  float4 vertex : POSITION; // 頂点座標
};

// 頂点シェーダからジオメトリシェーダに渡すデータ
struct v2g
{
  float4 pos : SV_POSITION; // 頂点座標
};

// 実体レンダリング時のジオメトリシェーダからフラグメントシェーダに渡すデータ
struct g2f_light
{
  float4 pos      : SV_POSITION;  // ローカル座標
  float3 normal   : NORMAL;       // 法線
  float4 worldPos : TEXCOORD0;    // ワールド座標
  half3 sh        : TEXCOORD3;    // SH
};

// 影のレンダリング時のジオメトリシェーダからフラグメントシェーダに渡すデータ
struct g2f_shadow
{
  float4 pos      : SV_POSITION;  // 座標
  float4 hpos     : TEXCOORD1;
};
//}


次に変数の定義をしています。@<br>{}

//list[kaiware_shader_arguments_define][変数の定義部分][cs]{
int _SegmentNum;

float _Scale;
float _Threashold;

float4 _DiffuseColor;
float3 _HalfSize;
float4x4 _Matrix;

float _EmissionIntensity;
half3 _EmissionColor;

half _Glossiness;
half _Metallic;

StructuredBuffer<float3> vertexOffset;
StructuredBuffer<int> cubeEdgeFlags;
StructuredBuffer<int2> edgeConnection;
StructuredBuffer<float3> edgeDirection;
StructuredBuffer<int> triangleConnectionTable;
//}

ここで定義している各種変数の中身は、C#側のRenderMesh()関数の中で、material.Set○○関数で受け渡しています。
MarchingCubesDefinesクラスのComputeBuffer達は、StructuredBuffer<○○>と型の呼び名が変わっています。


=== 頂点シェーダ


ほとんどの処理はジオメトリシェーダの方で行うので、頂点シェーダは凄くシンプルです。単純にメッシュから渡される頂点データをそのままジオメトリシェーダに渡しているだけです。


//list[kaiware_shader_vertex][頂点シェーダの実装部分][cs]{
// メッシュから渡ってくる頂点データ
struct appdata
{
  float4 vertex : POSITION; // 頂点座標
};

// 頂点シェーダからジオメトリシェーダに渡すデータ
struct v2g
{
  float4 pos : SV_POSITION; // 座標
};

// 頂点シェーダ
v2g vert(appdata v)
{
  v2g o = (v2g)0;
  o.pos = v.vertex;
  return o;
}
//}


ちなみに、頂点シェーダは実体と影で共通です。


=== 実体のジオメトリシェーダ


長いので分割しながら説明します。  


//list[kaiware_shader_geometry_1][ジオメトリシェーダーの関数宣言部分][cs]{
// 実体のジオメトリシェーダ
[maxvertexcount(15)]  // シェーダから出力する頂点の最大数の定義
void geom_light(point v2g input[1], 
                inout TriangleStream<g2f_light> outStream)
//}


まず、ジオメトリシェーダの宣言部です。  


#@#//emlist[][cs]{
#@#[maxvertexcount(15)]
#@#//}
@<code>{[maxvertexcount(15)]}はシェーダから出力する頂点の最大数の定義です。今回のマーチングキューブス法のアルゴリズムでは1グリッドにつき、三角ポリゴンが最大5つできるので、3*5で合計15個の頂点が出力されます。@<br>{}
そのため、maxvertexcountの()の中に15と記述します。


//list[kaiware_shader_geometry_2][グリッドの8つの角のスカラー値取得部分][cs]{
float cubeValue[8]; // グリッドの8つの角のスカラー値取得用の配列

// グリッドの8つの角のスカラー値を取得
for (i = 0; i < 8; i++) {
  cubeValue[i] = Sample(
                  pos.x + vertexOffset[i].x,
                  pos.y + vertexOffset[i].y,
                  pos.z + vertexOffset[i].z
  );
}
//}


posは、メッシュを作成する時にグリッド空間に配置した頂点の座標が入っています。vertexOffsetは、名前の通りposに加えるオフセット座標の配列です。

このループは、1頂点＝１つのグリッドの8つの角の座標のボリュームデータ中のスカラー値を取得しています。
vertexOffsetは、グリッドの角の順番を指しています。



//image[marching_cubes_005@4x][グリッドの角の座標の順番]{
//}



//list[kaiware_shader_sampling][サンプリング関数部分][cs]{
// サンプリング関数
float Sample(float x, float y, float z) {

  // 座標がグリッド空間からはみ出してたいないか？
  if ((x <= 1) || 
      (y <= 1) || 
      (z <= 1) || 
      (x >= (_SegmentNum - 1)) || 
      (y >= (_SegmentNum - 1)) || 
      (z >= (_SegmentNum - 1))
     )
    return 0;

  float3 size = float3(_SegmentNum, _SegmentNum, _SegmentNum);

  float3 pos = float3(x, y, z) / size;

  float3 spPos;
  float result = 0;

  // ３つの球の距離関数
  for (int i = 0; i < 3; i++) {
    float sp = -sphere(
      pos - float3(0.5, 0.25 + 0.25 * i, 0.5), 
      0.1 + (sin(_Time.y * 8.0 + i * 23.365) * 0.5 + 0.5) * 0.025) + 0.5;
    result = smoothMax(result, sp, 14);
  }

  return result;
}
//}


ボリュームデータから指定した座標のスカラー値を取ってくる関数です。今回は膨大な3Dボリュームデータではなく、距離関数を使ったシンプルなアルゴリズムでスカラー値を算出します。


====[column] 距離関数について


今回マーチングキューブス法で描画する3次元形状は、@<b>{「距離関数」}と言うものを使って定義します。

ここでいう距離関数とは、ざっくり説明すると「距離の条件を満たす関数」です。

例えば、球体の距離関数は、以下になります。  


//list[kaiware_distance_function][球体の距離関数][cs]{
inline float sphere(float3 pos, float radius)
{
    return length(pos) - radius;
}
//}


pos には、座標が入るのですが、球体の中心座標を原点(0,0,0)とした場合で考えます。radiusは半径です。

length(pos)で長さを求めていますが、これは原点とposまでの距離で、それを半径radiusで引くので、半径以下の長さの場合、当たり前ですが負の値になります。

つまり、座標posを渡して負の値が返ってきた場合は、「座標は球体の中にいる」という判定ができます。

距離関数のメリットは、数行のシンプルな計算式で図形を表現できるので、プログラムが小さくしやすいところです。その他の距離関数についての情報は、Inigo Quilez氏のサイトでたくさん紹介されています。

@<href>{http://iquilezles.org/www/articles/distfunctions/distfunctions.htm,http://iquilezles.org/www/articles/distfunctions/distfunctions.htm}

====[/column]


//list[kaiware_sphere][3つの球の距離関数を合成したもの][cs]{
// 3つの球の距離関数
for (int i = 0; i < 3; i++) {
  float sp = -sphere(
    pos - float3(0.5, 0.25 + 0.25 * i, 0.5), 
    0.1 + (sin(_Time.y * 8.0 + i * 23.365) * 0.5 + 0.5) * 0.025) + 0.5;
  result = smoothMax(result, sp, 14);
}
//}


今回は、グリッドの1マスの8つの角（頂点）をposとして使っています。球体の中心からの距離を、そのままボリュームデータの濃度として扱います。

後述しますが、閾値が0.5以上の時にポリゴン化するため、符号を反転しています。また、座標を微妙にずらして3つの球体との距離を求めています。  


//list[kaiware_shader_smoothmax][smoothMax関数][cs]{
float smoothMax(float d1, float d2, float k)
{
  float h = exp(k * d1) + exp(k * d2);
  return log(h) / k;
}
//}


smoothMaxは、距離関数の結果をいい感じにブレンドする関数です。これを使って3つの球体をメタボールのように融合させることが出来ます。  


//list[kaiware_shader_flagindex][閾値チェック][cs]{
// グリッドの８つの角の値が閾値を超えているかチェック
for (i = 0; i < 8; i++) {
  if (cubeValue[i] <= _Threashold) {
    flagIndex |= (1 << i);
  }
}

int edgeFlags = cubeEdgeFlags[flagIndex];

// 空か完全に満たされている場合は何も描画しない
if ((edgeFlags == 0) || (edgeFlags == 255)) {
  return;
}
//}


グリッドの角のスカラー値が閾値を越えていたら、flagIndexにビットを立てていきます。そのflagIndexをインデックスとして、cubeEdgeFlags配列からポリゴンを生成するための情報を取り出してedgeFlagsに格納しています。
グリッドの全ての角が閾値未満か閾値以上の場合は、完全に中か外なのでポリゴンは生成しません。  


//list[kaiware_shader_offset][ポリゴンの頂点座標計算][cs]{
float offset = 0.5;
float3 vertex;
for (i = 0; i < 12; i++) {
  if ((edgeFlags & (1 << i)) != 0) {
    // 角同士の閾値のオフセットを取得
    offset = getOffset(
               cubeValue[edgeConnection[i].x], 
               cubeValue[edgeConnection[i].y], _
               Threashold
             );

    // オフセットを元に頂点の座標を補完
    vertex = vertexOffset[edgeConnection[i].x] 
             + offset * edgeDirection[i];

    edgeVertices[i].x = pos.x + vertex.x * _Scale;
    edgeVertices[i].y = pos.y + vertex.y * _Scale;
    edgeVertices[i].z = pos.z + vertex.z * _Scale;

    // 法線計算（Sampleし直すため、スケールを掛ける前の頂点座標が必要）
    edgeNormals[i] = getNormal(
                        defpos.x + vertex.x, 
                        defpos.y + vertex.y, 
                        defpos.z + vertex.z
                     );
  }
}
//}


ポリゴンの頂点座標を計算している箇所です。先程の、edgeFlagsのビットを見て、グリッドの辺上に置くポリゴンの頂点座標を計算しています。

getOffsetは、グリッドの2つの角のスカラー値と閾値から、今の角から次の角までの割合(offset)を出しています。今の角の座標から、次の角の方向へoffset分ずらすことで、最終的になめらかなポリゴンになります。

getNormalでは、サンプリングし直して勾配を出して法線を算出しています。


//list[kaiware_shader_make_polygon][頂点を連結してポリゴンを作る][cs]{
// 頂点を連結してポリゴンを作成
int vindex = 0;
int findex = 0;
// 最大5つの三角形ができる
for (i = 0; i < 5; i++) {
  findex = flagIndex * 16 + 3 * i;
  if (triangleConnectionTable[findex] < 0)
    break;

  // 三角形を作る
  for (j = 0; j < 3; j++) {
    vindex = triangleConnectionTable[findex + j];

    // Transform行列を掛けてワールド座標に変換
    float4 ppos = mul(_Matrix, float4(edgeVertices[vindex], 1));
    o.pos = UnityObjectToClipPos(ppos);

    float3 norm = UnityObjectToWorldNormal(
                    normalize(edgeNormals[vindex])
                  );
    o.normal = normalize(mul(_Matrix, float4(norm,0)));

    outStream.Append(o);  // ストリップに頂点を追加
  }
  outStream.RestartStrip(); // 一旦区切って次のプリミティブストリップを開始
}
//}


先程求めた頂点座標群を繋いでポリゴンを作っている箇所です。triangleConnectionTable配列に接続する頂点のインデックスが入っています。頂点座標にTransformの行列を掛けてワールド座標に変換し、UnityObjectToClipPos()でスクリーン座標に変換しています。

また、UnityObjectToWorldNormal()で法線もワールド座標系に変換しています。これらの頂点と法線は、次のフラグメントシェーダでライティングに使います。

TriangleStream.Append()やRestartStrip()は、ジオメトリシェーダ用の特殊な関数です。
Append()は、現在のストリップに頂点データを追加します。RestartStrip()は、新しいストリップを作成します。TriangleStreamなので1つのストリップには3つまでAppendするイメージです。


=== 実体のフラグメントシェーダ


UnityのGI(グローバルイル・ミネーション)などのライティングを反映させるため、Generate code後のSurfaceShaderのライティング処理部分を移植します。


//list[kaiware_shader_fragment_define][フラグメントシェーダの定義][cs]{
// 実体のフラグメントシェーダ
void frag_light(g2f_light IN,
  out half4 outDiffuse        : SV_Target0,
  out half4 outSpecSmoothness : SV_Target1,
  out half4 outNormal         : SV_Target2,
  out half4 outEmission       : SV_Target3)
//}


G-Bufferに出力するため出力(SV_Target)が4つあります。  


//list[kaiware_shader_surface][SurfaceOutputStandard構造体の初期化][cs]{
#ifdef UNITY_COMPILER_HLSL
  SurfaceOutputStandard o = (SurfaceOutputStandard)0;
#else
  SurfaceOutputStandard o;
#endif
  o.Albedo = _DiffuseColor.rgb;
  o.Emission = _EmissionColor * _EmissionIntensity;
  o.Metallic = _Metallic;
  o.Smoothness = _Glossiness;
  o.Alpha = 1.0;
  o.Occlusion = 1.0;
  o.Normal = normal;
//}


あとで使うSurfaceOutputStandard構造体に、色や光沢感などのパラメータをセットします。


//list[kaiware_shader_light][GI関係の処理][cs]{
// Setup lighting environment
UnityGI gi;
UNITY_INITIALIZE_OUTPUT(UnityGI, gi);
gi.indirect.diffuse = 0;
gi.indirect.specular = 0;
gi.light.color = 0;
gi.light.dir = half3(0, 1, 0);
gi.light.ndotl = LambertTerm(o.Normal, gi.light.dir);

// Call GI (lightmaps/SH/reflections) lighting function
UnityGIInput giInput;
UNITY_INITIALIZE_OUTPUT(UnityGIInput, giInput);
giInput.light = gi.light;
giInput.worldPos = worldPos;
giInput.worldViewDir = worldViewDir;
giInput.atten = 1.0;

giInput.ambient = IN.sh;

giInput.probeHDR[0] = unity_SpecCube0_HDR;
giInput.probeHDR[1] = unity_SpecCube1_HDR;

#if UNITY_SPECCUBE_BLENDING || UNITY_SPECCUBE_BOX_PROJECTION
// .w holds lerp value for blending
giInput.boxMin[0] = unity_SpecCube0_BoxMin; 
#endif

#if UNITY_SPECCUBE_BOX_PROJECTION
giInput.boxMax[0] = unity_SpecCube0_BoxMax;
giInput.probePosition[0] = unity_SpecCube0_ProbePosition;
giInput.boxMax[1] = unity_SpecCube1_BoxMax;
giInput.boxMin[1] = unity_SpecCube1_BoxMin;
giInput.probePosition[1] = unity_SpecCube1_ProbePosition;
#endif

LightingStandard_GI(o, giInput, gi);
//}


GI関係の処理です。UnityGIInputに初期値を入れて、LightnintStandard_GI()で計算したGIの結果をUnityGIに書き込んでいます。  


//list[kaiware_shader_gi][光の反射具合の計算][cs]{
// call lighting function to output g-buffer
outEmission = LightingStandard_Deferred(o, worldViewDir, gi, 
                                        outDiffuse, 
                                        outSpecSmoothness, 
                                        outNormal);
outDiffuse.a = 1.0;

#ifndef UNITY_HDR_ON
outEmission.rgb = exp2(-outEmission.rgb);
#endif
//}


諸々の計算結果を LightingStandard_Deferred() に渡して光の反射具合を計算して、Emissionバッファに書き込みます。HDRの場合は、expで圧縮される部分を挟んでから書き込みます。


=== 影のジオメトリシェーダ


実体のジオメトリシェーダとほとんど同じです。違いがある所だけ解説します。  


//list[kaiware_shader_geometry_shadow][影のジオメトリシェーダ][cs]{
int vindex = 0;
int findex = 0;
for (i = 0; i < 5; i++) {
  findex = flagIndex * 16 + 3 * i;
  if (triangleConnectionTable[findex] < 0)
    break;

  for (j = 0; j < 3; j++) {
    vindex = triangleConnectionTable[findex + j];

    float4 ppos = mul(_Matrix, float4(edgeVertices[vindex], 1));

    float3 norm;
    norm = UnityObjectToWorldNormal(normalize(edgeNormals[vindex]));

    float4 lpos1 = mul(unity_WorldToObject, ppos);
    o.pos = UnityClipSpaceShadowCasterPos(lpos1, 
                                            normalize(
                                              mul(_Matrix, 
                                                float4(norm, 0)
                                              )
                                            )
                                          );
    o.pos = UnityApplyLinearShadowBias(o.pos);
    o.hpos = o.pos;

    outStream.Append(o);
  }
  outStream.RestartStrip();
}
//}


UnityClipSpaceShadowCasterPos()とUnityApplyLinearShadowBias()で頂点座標を影の投影先の座標に変換します。


=== 影のフラグメントシェーダ

//list[kaiware_shader_fragment_shadow][影のフラグメントシェーダ][cs]{
// 影のフラグメントシェーダ
fixed4 frag_shadow(g2f_shadow i) : SV_Target
{
  return i.hpos.z / i.hpos.w;
}
//}


短すぎて説明するところがないです。実は return 0; でも正常に影が描画されます。Unityが中でいい感じにやってくれているんでしょうか？


== 完成


実行するとこんな感じの絵が出てくるはずです。

//image[marching_cubes_006][うねうね][scale=0.25]{
//}


また、距離関数を組み合わせるといろいろな形が作れます。  

//image[marching_cubes_007][かいわれーい][scale=0.25]{
//}

== まとめ

今回は簡略化のために距離関数を使いましたが、他にも3Dテクスチャにボリュームデータを書き込んだものを使ったり、いろいろな三次元データを可視化するのにマーチングキューブス法は使えると思います。@<br>{}
ゲーム用途では、地形を掘ったり盛ったりできるASTORONEER@<fn>{astroneer}のようなゲームも作れるかもしれません。@<br>{}
みなさんもマーチングキューブス法でいろいろな表現を模索してみてください！

== 参考

 * Polygonising a scalar field - http://paulbourke.net/geometry/polygonise/
 * modeling with distance functions - 
http://iquilezles.org/www/articles/distfunctions/distfunctions.htm

//footnote[astroneer][ASTRONEER http://store.steampowered.com/app/361420/ASTRONEER/?l=japanese]
