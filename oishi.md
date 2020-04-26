# 群のシミュレーションのGPU実装

## はじめに
この章では、ComputeShaderを使ったBoidsアルゴリズムを用いた群のシミュレーションの実装について解説します。
鳥や魚、その他の陸上動物は時として群を作ります。この群の動きには規則性と複雑性が見られ、ある種の美しさを持っており人を惹きつけてきました。
コンピュターグラフィックスにおいては、それらの個体の振る舞いを一つ一つ人の手で制御することは現実的でなく、Boidsと呼ばれる群を作るためのアルゴリズムが考案されました。このシミュレーションアルゴリズムは、いくつかのシンプルな規則で構成されており実装も容易ですが、単純な実装では、すべての個体との位置関係を調べる必要があり、個体数が増えると2乗で計算量が増加してしまいます。多くの個体を制御したいという場合、CPUによる実装では非常に困難です。そこで、GPUによる強力な並列計算能力を利用したいと思います。Unityには、GPUによるこのような汎用的な計算（GPGPU）を行うため、ComputeShaderというシェーダプログラムが用意されています。GPUには共有メモリと呼ばれる特殊な記憶領域が組み込まれており、ComputeShaderを用いると、このメモリを有効に活用することができます。また、UnityにはGPUインスタンシングという高度なレンダリング機能があり、任意のメッシュを効率的に描画することが可能です。このような、UnityのGPUの計算能力を生かした機能を使い、多数のBoidオブジェクトを制御し描画するプログラムを紹介いたします。

## Boidsのアルゴリズム
Boidsと呼ばれる群のシミュレーションアルゴリズムは、Craig Reynoldsによって1986年に開発され、翌年1987年のACM SIGGRAPHに「Flocks, Herds, and Schools: A Distributed Behavioral Model」というタイトルの論文として発表されました。

Reynoldsは、群れというものは、それぞれの個体が視覚や聴覚などの知覚によって、周囲の他の個体の位置や動く方向に基づいて自身の行動を修正することにより、結果として複雑な振る舞いを生み出している、ということに着目します。

余談ですが、Boidsというのは、Birdoid（鳥っぽいもの）という言葉が省略されたものが語の起源のようです。また、日本語では「群れ」と1つにくくられてしまいますが、英語では、鳥の群れを「Flocking」、陸上生物の群れを「Herd」、魚の群れを「School」と表現します。

基本的なBoidsアルゴリズムは、以下の３つのシンプルなルールによって構成されます。

##### 1.分離（Separation） 
  
ある一定の距離内にある個体と密集することを避けるように動く

##### 2.整列（Alignment）
	
ある一定の距離内にある個体が向いている方向の平均に向かおうと動く

##### 3.結合（Cohesion）

ある一定の距離内にある個体の平均位置に動く

![Boidsの基本的なルール](images/oishi/boids-rules.png)

これらのルールに従って、個々の動きを制御することにより、群れの動きをプログラムすることができます。

## サンプルプログラム
### リポジトリ
[https://github.com/IndieVisualLab/UnityGraphicsProgramming](https://github.com/IndieVisualLab/UnityGraphicsProgramming)

本書のサンプルUnityプロジェクトにある、Assets/**BoidsSimulationOnGPU**フォルダ内の**BoidsSimulationOnGPU.unity**シーンデータを開いてください。

### 実行条件
本章で紹介するプログラムは、ComputeShader、GPUインスタンシングを使用しています。

ComputeShaderは、以下のプラットフォームまたはAPIで動作します。

* DirectX11、またはDirectX12グラフィックスAPIおよびシェーダモデル5.0GPUを搭載したWindowsおよびWindowsストアアプリ
* macOSとMetalグラフィックスAPIを使用したiOS
* Vulkan APIを搭載したAndroid、Linux、Windowsプラットフォーム
* 最新のOpenGLプラットフォーム（LinuxまたはWindowsではOpenGL 4.3、AndroidではOpenGL ES 3.1）。（MacOSXはOpenGL4.3をサポートしていないので注意してください）
* 現段階で一般的に使用されているコンソール機（Sony PS4、Microsoft Xbox One）


GPUインスタンシングは以下のプラットフォームまたはAPIで利用可能です。

* Windows上のDirectX 11およびDirectX 12
* Windows、MacOS、Linux、iOS、Android上のOpenGLコア4.1 + / ES3.0 +
* macOSとiOS上のMetal
* WindowsとAndroidのVulkan
* プレイステーション4とXbox One
* WebGL（WebGL 2.0 APIが必要）

GPUインスタンシング時、Graphics.DrawMeshInstacedIndirectメソッドを使用しています。そのため、Unityのバージョンは5.6以降である必要があります。

## 実装コードの解説
本サンプルプログラムは以下のコードで構成されます。

|スクリプト名|機能|
|:--|:--|
|GPUBoids.cs|Boidsのシミュレーションを行うComputeShaderを制御するスクリプト|
|Boids.compute|Boidsのシミュレーションを行うComputeShader|
|BoidsRender.cs|Boidsを描画するシェーダを制御するC#スクリプト|
|BoidsRender.shader|GPUインスタンシングによってオブジェクトを描画するためのシェーダ|

スクリプトやマテリアルリソースなどはこのようにセットします

![UnityEditor上での設定](images/oishi/editor-boids.png)

### GPUBoids.cs
このコードでは、Boidsシミュレーションのパラメータや、GPU上での計算のために必要なバッファや計算命令を記述したComputeShaderなどの管理を行います。
	
```csharp

using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class GPUBoids : MonoBehaviour
{
    // Boidデータの構造体
    [System.Serializable]
    struct BoidData 
    {
        public Vector3 Velocity; // 速度
        public Vector3 Position; // 位置
    }
    // スレッドグループのスレッドのサイズ
    const int SIMULATION_BLOCK_SIZE = 256;

    #region Boids Parameters
    // 最大オブジェクト数
    [Range(256, 32768)]
    public int MaxObjectNum = 16384;

    // 結合を適用する他の個体との半径
    public float CohesionNeighborhoodRadius  = 2.0f;
    // 整列を適用する他の個体との半径
    public float AlignmentNeighborhoodRadius = 2.0f;
    // 分離を適用する他の個体との半径
    public float SeparateNeighborhoodRadius  = 1.0f;

    // 速度の最大値
    public float MaxSpeed        = 5.0f;
    // 操舵力の最大値
    public float MaxSteerForce   = 0.5f;

    // 結合する力の重み
    public float CohesionWeight  = 1.0f;
    // 整列する力の重み
    public float AlignmentWeight = 1.0f;
    // 分離する力の重み
    public float SeparateWeight  = 3.0f;

    // 壁を避ける力の重み
    public float AvoidWallWeight = 10.0f;
    
    // 壁の中心座標   
    public Vector3 WallCenter = Vector3.zero;
    // 壁のサイズ
    public Vector3 WallSize = new Vector3(32.0f, 32.0f, 32.0f);
    #endregion

    #region Built-in Resources
    // Boidsシミュレーションを行うComputeShaderの参照
    public ComputeShader BoidsCS;
    #endregion

    #region Private Resources
    // Boidの操舵力（Force）を格納したバッファ
    ComputeBuffer _boidForceBuffer;
    // Boidの基本データ（速度, 位置, Transformなど）を格納したバッファ
    ComputeBuffer _boidDataBuffer;
    #endregion

    #region Accessors
    // Boidの基本データを格納したバッファを取得
    public ComputeBuffer GetBoidDataBuffer()
    {
        return this._boidDataBuffer != null ? this._boidDataBuffer : null;
    }

    // オブジェクト数を取得
    public int GetMaxObjectNum()
    {
        return this.MaxObjectNum;
    }

    // シミュレーション領域の中心座標を返す
    public Vector3 GetSimulationAreaCenter()
    {
        return this.WallCenter;
    }

    // シミュレーション領域のボックスのサイズを返す
    public Vector3 GetSimulationAreaSize()
    {
        return this.WallSize;
    }
    #endregion

    #region MonoBehaviour Functions
    void Start()
    {
        // バッファを初期化
        InitBuffer();
    }

    void Update()
    {
        // シミュレーション
        Simulation();
    }
    
    void OnDestroy()
    {
        // バッファを破棄
        ReleaseBuffer(); 
    }

    void OnDrawGizmos()
    {
        // デバッグとしてシミュレーション領域をワイヤーフレームで描画
        Gizmos.color = Color.cyan;
        Gizmos.DrawWireCube(WallCenter, WallSize);
    }
    #endregion

    #region Private Functions
    // バッファを初期化
    void InitBuffer()
    {
        // バッファを初期化
        _boidDataBuffer  = new ComputeBuffer(MaxObjectNum, 
            Marshal.SizeOf(typeof(BoidData)));        
        _boidForceBuffer = new ComputeBuffer(MaxObjectNum, 
            Marshal.SizeOf(typeof(Vector3)));

        // Boidデータ, Forceバッファを初期化
        var forceArr = new Vector3[MaxObjectNum];
        var boidDataArr = new BoidData[MaxObjectNum];
        for (var i = 0; i < MaxObjectNum; i++)
        {
            forceArr[i] = Vector3.zero;
            boidDataArr[i].Position = Random.insideUnitSphere * 1.0f;
            boidDataArr[i].Velocity = Random.insideUnitSphere * 0.1f;
        }
        _boidForceBuffer.SetData(forceArr);
        _boidDataBuffer.SetData(boidDataArr);
        forceArr    = null;
        boidDataArr = null;
    }

    // シミュレーション
    void Simulation()
    {
        ComputeShader cs = BoidsCS;
        int id = -1;

        // スレッドグループの数を求める
        int threadGroupSize = Mathf.CeilToInt(MaxObjectNum 
            / SIMULATION_BLOCK_SIZE);
        
        // 操舵力を計算
        id = cs.FindKernel("ForceCS"); // カーネルIDを取得
        cs.SetInt("_MaxBoidObjectNum", MaxObjectNum);
        cs.SetFloat("_CohesionNeighborhoodRadius",  
            CohesionNeighborhoodRadius);
        cs.SetFloat("_AlignmentNeighborhoodRadius", 
            AlignmentNeighborhoodRadius);
        cs.SetFloat("_SeparateNeighborhoodRadius",  
            SeparateNeighborhoodRadius);
        cs.SetFloat("_MaxSpeed", MaxSpeed);
        cs.SetFloat("_MaxSteerForce", MaxSteerForce);
        cs.SetFloat("_SeparateWeight", SeparateWeight);
        cs.SetFloat("_CohesionWeight", CohesionWeight);
        cs.SetFloat("_AlignmentWeight", AlignmentWeight);
        cs.SetVector("_WallCenter", WallCenter);
        cs.SetVector("_WallSize", WallSize);
        cs.SetFloat("_AvoidWallWeight", AvoidWallWeight);
        cs.SetBuffer(id, "_BoidDataBufferRead", _boidDataBuffer);
        cs.SetBuffer(id, "_BoidForceBufferWrite", _boidForceBuffer);
        cs.Dispatch(id, threadGroupSize, 1, 1); // ComputeShaderを実行

        // 操舵力から、速度と位置を計算
        id = cs.FindKernel("IntegrateCS"); // カーネルIDを取得
        cs.SetFloat("_DeltaTime", Time.deltaTime);
        cs.SetBuffer(id, "_BoidForceBufferRead", _boidForceBuffer);
        cs.SetBuffer(id, "_BoidDataBufferWrite", _boidDataBuffer);
        cs.Dispatch(id, threadGroupSize, 1, 1); // ComputeShaderを実行
    }

    // バッファを解放
    void ReleaseBuffer()
    {
        if (_boidDataBuffer != null)
        {
            _boidDataBuffer.Release();
            _boidDataBuffer = null;
        }

        if (_boidForceBuffer != null)
        {
            _boidForceBuffer.Release();
            _boidForceBuffer = null;
        }
    }
    #endregion
}

```

##### ComputeBufferの初期化

InitBuffer関数では、GPU上で計算を行う際に使用するバッファを宣言しています。
GPU上で計算するためのデータを格納するバッファとして、ComputeBufferというクラスを使用します。ComputeBufferはComputeShaderのためにデータを格納するデータバッファです。C#スクリプトからGPU上のメモリバッファに対して読み込みや書き込みができるようになります。ComputeShader側では、ComputeBufferはHLSLのRWStructuredBuffer<T>とStructuredBuffer<T>にマッピングされます。
初期化時の引数には、バッファの要素の数と、要素1つのサイズ（バイト数）を渡します。Marshal.SizeOf()メソッドを使用することで、型のサイズ（バイト数）を取得することができます。ComputeBufferでは、SetData()を用いて、
構造体の配列の値をセットすることができます。

##### ComputeShaderに記述した関数の実行

Simulation関数では、ComputeShaderに必要なパラメータを渡し、計算命令を発行します。

ComputeShaderに記述された、実際にGPUに計算をさせる関数はカーネルと呼ばれます。このカーネルの実行単位をスレッドと言い、GPUアーキテクチャに即した並列計算処理を行うために、任意の数まとめてグループとして扱い、それらはスレッドグループと呼ばれます。
このスレッドの数とスレッドグループ数の積が、Boidの個体数と同じかそれを超えるように設定します。

カーネルは、ComputeShaderスクリプト内で #pragma kernelディレクティブを用いて指定されます。これにはそれぞれIDが割り当てられており、C#スクリプトからはFindKernelメソッドを用いることで、このIDを取得することができます。

SetFloatメソッド、SetVectorメソッド、SetBufferメソッドなどを使用し、シミュレーションに必要なパラメータやバッファをComputeShaderに渡します。バッファやテクスチャをセットするときにはカーネルIDが必要になります。

Dispatchメソッドを実行することで、ComputeShaderに定義したカーネルをGPUで計算処理を行うように命令を発行します。引数には、カーネルIDとスレッドグループの数を指定します。

### Boids.compute
GPUへの計算命令を記述します。カーネルは2つで、1つは操舵力を計算するもの、もう1つは、その力を適用させ、速度や位置を更新するものです。

```

// カーネル関数を指定
#pragma kernel ForceCS      // 操舵力を計算
#pragma kernel IntegrateCS  // 速度, 位置を計算

// Boidデータの構造体
struct BoidData
{
    float3 velocity; // 速度
    float3 position; // 位置
};

// スレッドグループのスレッドのサイズ
#define SIMULATION_BLOCK_SIZE 256

// Boidデータのバッファ（読み取り用）
StructuredBuffer<BoidData>   _BoidDataBufferRead;
// Boidデータのバッファ（読み取り, 書き込み用）
RWStructuredBuffer<BoidData> _BoidDataBufferWrite;
// Boidの操舵力のバッファ（読み取り用）
StructuredBuffer<float3>     _BoidForceBufferRead;
// Boidの操舵力のバッファ（読み取り, 書き込み用）
RWStructuredBuffer<float3>   _BoidForceBufferWrite;

int _MaxBoidObjectNum; // Boidオブジェクト数

float _DeltaTime;      // 前フレームから経過した時間

float _SeparateNeighborhoodRadius;  // 分離を適用する他の個体との距離
float _AlignmentNeighborhoodRadius; // 整列を適用する他の個体との距離
float _CohesionNeighborhoodRadius;  // 結合を適用する他の個体との距離

float _MaxSpeed;        // 速度の最大値
float _MaxSteerForce;   // 操舵する力の最大値

float _SeparateWeight;  // 分離適用時の重み
float _AlignmentWeight; // 整列適用時の重み
float _CohesionWeight;  // 結合適用時の重み

float4 _WallCenter;      // 壁の中心座標
float4 _WallSize;        // 壁のサイズ
float  _AvoidWallWeight; // 壁を避ける強さの重み


// ベクトルの大きさを制限する
float3 limit(float3 vec, float max)
{
    float length = sqrt(dot(vec, vec)); // 大きさ
    return (length > max && length > 0) ? vec.xyz * (max / length) : vec.xyz;
}

// 壁に当たった時に逆向きの力を返す
float3 avoidWall(float3 position)
{
    float3 wc = _WallCenter.xyz;
    float3 ws = _WallSize.xyz;
    float3 acc = float3(0, 0, 0);
    // x
    acc.x = (position.x < wc.x - ws.x * 0.5) ? acc.x + 1.0 : acc.x;
    acc.x = (position.x > wc.x + ws.x * 0.5) ? acc.x - 1.0 : acc.x;
    
    // y
    acc.y = (position.y < wc.y - ws.y * 0.5) ? acc.y + 1.0 : acc.y;
    acc.y = (position.y > wc.y + ws.y * 0.5) ? acc.y - 1.0 : acc.y;
    
    // z
    acc.z = (position.z < wc.z - ws.z * 0.5) ? acc.z + 1.0 : acc.z;
    acc.z = (position.z > wc.z + ws.z * 0.5) ? acc.z - 1.0 : acc.z;

    return acc;
}

// シェアードメモリ Boidデータ格納用
groupshared BoidData boid_data[SIMULATION_BLOCK_SIZE];

// 操舵力の計算用カーネル関数
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void ForceCS
(
    uint3 DTid : SV_DispatchThreadID, // スレッド全体で固有のID
    uint3 Gid : SV_GroupID, // グループのID
    uint3 GTid : SV_GroupThreadID, // グループ内のスレッドID
    uint  GI : SV_GroupIndex // SV_GroupThreadIDを一次元にしたもの 0-255
)
{
    const unsigned int P_ID = DTid.x; // 自身のID
    float3 P_position = _BoidDataBufferRead[P_ID].position; // 自身の位置
    float3 P_velocity = _BoidDataBufferRead[P_ID].velocity; // 自身の速度

    float3 force = float3(0, 0, 0); // 操舵力を初期化

    float3 sepPosSum = float3(0, 0, 0); // 分離計算用 位置加算変数
    int sepCount = 0; // 分離のために計算した他の個体の数のカウント用変数

    float3 aliVelSum = float3(0, 0, 0); // 整列計算用 速度加算変数
    int aliCount = 0; // 整列のために計算した他の個体の数のカウント用変数

    float3 cohPosSum = float3(0, 0, 0); // 結合計算用 位置加算変数
    int cohCount = 0; // 結合のために計算した他の個体の数のカウント用変数

    // SIMULATION_BLOCK_SIZE（グループスレッド数）ごとの実行 (グループ数分実行)
    [loop]
    for (uint N_block_ID = 0; N_block_ID < (uint)_MaxBoidObjectNum;
        N_block_ID += SIMULATION_BLOCK_SIZE)
    {
        // SIMULATION_BLOCK_SIZE分のBoidデータを、シェアードメモリに格納
        boid_data[GI] = _BoidDataBufferRead[N_block_ID + GI];

        // すべてのグループ共有アクセスが完了し、
        // グループ内のすべてのスレッドがこの呼び出しに到達するまで、
        // グループ内のすべてのスレッドの実行をブロックする
        GroupMemoryBarrierWithGroupSync();

        // 他の個体との計算
        for (int N_tile_ID = 0; N_tile_ID < SIMULATION_BLOCK_SIZE; 
            N_tile_ID++)
        {
            // 他の個体の位置
            float3 N_position = boid_data[N_tile_ID].position;
            // 他の個体の速度
            float3 N_velocity = boid_data[N_tile_ID].velocity;

            // 自身と他の個体の位置の差
            float3 diff = P_position - N_position;
            // 自身と他の個体の位置の距離
            float  dist = sqrt(dot(diff, diff));   

            // --- 分離（Separation） ---
            if (dist > 0.0 && dist <= _SeparateNeighborhoodRadius)
            {
                // 他の個体の位置から自身へ向かうベクトル
                float3 repulse = normalize(P_position - N_position);
                // 自身と他の個体の位置の距離で割る（距離が遠ければ影響を小さく）
                repulse /= dist;
                sepPosSum += repulse; // 加算
                sepCount++;           // 個体数カウント
            }

            // --- 整列（Alignment） ---
            if (dist > 0.0 && dist <= _AlignmentNeighborhoodRadius)
            {
                aliVelSum += N_velocity; // 加算
                aliCount++;              // 個体数カウント
            }

            // --- 結合（Cohesion） ---
            if (dist > 0.0 && dist <= _CohesionNeighborhoodRadius)
            {
                cohPosSum += N_position; // 加算
                cohCount++;              // 個体数カウント 
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // 操舵力（分離）
    float3 sepSteer = (float3)0.0;
    if (sepCount > 0)
    {
        sepSteer = sepPosSum / (float)sepCount;     // 平均を求める
        sepSteer = normalize(sepSteer) * _MaxSpeed; // 最大速度に調整
        sepSteer = sepSteer - P_velocity;           // 操舵力を計算
        sepSteer = limit(sepSteer, _MaxSteerForce); // 操舵力を制限
    }

    // 操舵力（整列）
    float3 aliSteer = (float3)0.0;
    if (aliCount > 0)
    {
        aliSteer = aliVelSum / (float)aliCount; // 近い個体の速度の平均を求める
        aliSteer = normalize(aliSteer) * _MaxSpeed; // 最大速度に調整
        aliSteer = aliSteer - P_velocity;           // 操舵力を計算
        aliSteer = limit(aliSteer, _MaxSteerForce); // 操舵力を制限
    }
    // 操舵力（結合）
    float3 cohSteer = (float3)0.0;
    if (cohCount > 0)
    {
        // / 近い個体の位置の平均を求める
        cohPosSum = cohPosSum / (float)cohCount;
        cohSteer = cohPosSum - P_position; // 平均位置方向へのベクトルを求める
        cohSteer = normalize(cohSteer) * _MaxSpeed; // 最大速度に調整
        cohSteer = cohSteer - P_velocity;           // 操舵力を計算
        cohSteer = limit(cohSteer, _MaxSteerForce); // 操舵力を制限
    }
    force += aliSteer * _AlignmentWeight; // 操舵力に整列する力を加える
    force += cohSteer * _CohesionWeight;  // 操舵力に結合する力を加える
    force += sepSteer * _SeparateWeight;  // 操舵力に分離する力を加える

    _BoidForceBufferWrite[P_ID] = force; // 書き込み
}

// 速度, 位置計算用カーネル関数
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void IntegrateCS
(
    uint3 DTid : SV_DispatchThreadID // スレッド全体で固有のID
)
{
    const unsigned int P_ID = DTid.x; // インデックスを取得
                                                       
    BoidData b = _BoidDataBufferWrite[P_ID]; // 現在のBoidデータを読み込む
    float3 force = _BoidForceBufferRead[P_ID]; // 操舵力を読み込む
    
    // 壁に近づいたら反発する力を与える
    force += avoidWall(b.position) * _AvoidWallWeight; 

    b.velocity += force * _DeltaTime; // 操舵力を速度に適用
    b.velocity = limit(b.velocity, _MaxSpeed); // 速度を制限
    b.position += b.velocity * _DeltaTime; // 位置を更新
                                                       
    _BoidDataBufferWrite[P_ID] = b; // 計算結果を書き込む
}

```

#### 操舵力の計算
ForceCSカーネルでは、操舵力の計算を行います。

##### 共有メモリの活用
groupshared という記憶域修飾子をつけられた変数は共有メモリ（shared memory）に書き込まれるようになります。
共有メモリは多くのデータ量を書き込むことはできませんが、レジスタに近く配置されており非常に高速にアクセスができます。
この共有メモリはスレッドグループ内で共有することができます。SIMULATION\_BLOCK\_SIZE分の他の個体の情報をまとめて共有メモリに書き込んでおいて、同一スレッドグループ内で高速に読みこむことができるようにすることで、他の個体との位置関係を考慮した計算を効率的に行っていきます。

![GPUの基本的なアーキテクチャ](images/oishi/gpu-architecture.png)

###### GroupMemoryBarrierWithGroupSync()
共有メモリに書き込まれたデータにアクセスする時は、GroupMemoryBarrierWithGroupSync()メソッドを記述し、スレッドグループ内のすべてのスレッドの処理の同期をとっておく必要があります。
GroupMemoryBarrierWithGroupSync()は、スレッドグループ内のすべてのスレッドが、この呼び出しに到達するまで、グループ内のすべてのスレッドの実行をブロックします。これにより、すべてのスレッドでboid\_data配列の初期化が適切に終わっていることが保証されるようにします。

##### 他の個体との距離によって操舵力を計算
###### 分離（Separation）
指定した距離より近い個体があった場合、その個体の位置から自身の位置へ向かうベクトルを求め、正規化します。そのベクトルを、距離の値で割ることで、近ければより避けるように、遠ければ小さく避けるように重みをつけ他の個体と衝突しないようにする力として加算していきます。全ての個体との計算が終わったら、その値を用いて、現在の速度との関係から操舵力を求めます。

###### 整列（Alignment）
指定した距離より近い個体があった場合、その個体の速度（Velocity）を足し合わせていき、同時にその個体数をカウントしていき、それらの値で、近い個体の速度（つまり向いている方向）の平均を求めます。全ての個体との計算が終わったら、その値を用いて、現在の速度との関係から操舵力を求めます。

###### 結合（Cohesion）
指定した距離より近い個体があった場合、その個体の位置を加算していき、同時にその個体数をカウントしていき、それらの値で、近い個体の位置の平均（重心）を求めます。さらに、そこへ向かうベクトルを求め、現在の速度との関係から操舵力を求めます。

##### 個々のBoidの速度と位置の更新
IntegrateCSカーネルでは、ForceCS()で求めた操舵力を元に、Boidの速度と位置を更新します。
AvoidWallでは、指定したエリアの外に出ようとした場合、逆向きの力を与え領域の内部に留まるようにしています。


### BoidsRender.cs
このスクリプトでは、Boidsシミュレーションで得られた結果を、指定したメッシュで描画することを行います。

```csharp

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// 同GameObjectに、GPUBoidsコンポーネントがアタッチされていることを保証
[RequireComponent(typeof(GPUBoids))]
public class BoidsRender : MonoBehaviour
{
    #region Paremeters
    // 描画するBoidsオブジェクトのスケール
    public Vector3 ObjectScale = new Vector3(0.1f, 0.2f, 0.5f);
    #endregion

    #region Script References
    // GPUBoidsスクリプトの参照
    public GPUBoids GPUBoidsScript;
    #endregion

    #region Built-in Resources
    // 描画するメッシュの参照
    public Mesh InstanceMesh;
    // 描画のためのマテリアルの参照
    public Material InstanceRenderMaterial;
    #endregion

    #region Private Variables
    // GPUインスタンシングのための引数（ComputeBufferへの転送用）
    // インスタンスあたりのインデックス数, インスタンス数, 
    // 開始インデックス位置, ベース頂点位置, インスタンスの開始位置
    uint[] args = new uint[5] { 0, 0, 0, 0, 0 };
    // GPUインスタンシングのための引数バッファ
    ComputeBuffer argsBuffer;
    #endregion

    #region MonoBehaviour Functions
    void Start ()
    {
        // 引数バッファを初期化
        argsBuffer = new ComputeBuffer(1, args.Length * sizeof(uint), 
            ComputeBufferType.IndirectArguments);
    }
    
    void Update ()
    {
        // メッシュをインスタンシング
        RenderInstancedMesh();
    }

    void OnDisable()
    {
        // 引数バッファを解放
        if (argsBuffer != null)
            argsBuffer.Release();
        argsBuffer = null;
    }
    #endregion

    #region Private Functions
    void RenderInstancedMesh()
    {
        // 描画用マテリアルがNull, または, GPUBoidsスクリプトがNull,
        // またはGPUインスタンシングがサポートされていなければ, 処理をしない
        if (InstanceRenderMaterial == null || GPUBoidsScript == null || 
            !SystemInfo.supportsInstancing)
            return;

        // 指定したメッシュのインデックス数を取得
        uint numIndices = (InstanceMesh != null) ? 
            (uint)InstanceMesh.GetIndexCount(0) : 0;
        // メッシュのインデックス数をセット
        args[0] = numIndices; 
        // インスタンス数をセット
        args[1] = (uint)GPUBoidsScript.GetMaxObjectNum(); 
        argsBuffer.SetData(args); // バッファにセット

        // Boidデータを格納したバッファをマテリアルにセット
        InstanceRenderMaterial.SetBuffer("_BoidDataBuffer", 
            GPUBoidsScript.GetBoidDataBuffer());
        // Boidオブジェクトスケールをセット
        InstanceRenderMaterial.SetVector("_ObjectScale", ObjectScale);
        // 境界領域を定義
        var bounds = new Bounds
        (
            GPUBoidsScript.GetSimulationAreaCenter(), // 中心
            GPUBoidsScript.GetSimulationAreaSize()    // サイズ
        );
        // メッシュをGPUインスタンシングして描画
        Graphics.DrawMeshInstancedIndirect
        (
            InstanceMesh,           // インスタンシングするメッシュ
            0,                      // submeshのインデックス
            InstanceRenderMaterial, // 描画を行うマテリアル 
            bounds,                 // 境界領域
            argsBuffer              // GPUインスタンシングのための引数のバッファ 
        );
    }
    #endregion
}

```

#### GPUインスタンシング
大量の同一のMeshを描画したい時、一つ一つGameObjectを生成するのでは、ドローコールが上がり描画負荷が増大していきます。また、ComputeShaderでの計算結果をCPUメモリに転送するコストが高く、高速に処理を行いたい場合、GPUでの計算結果をそのまま描画用シェーダに渡し描画処理をさせることが必要です。UnityのGPUインスタンシングを使えば、不要なGameObjectの生成を行うことなく、大量の同一のMeshを少ないドローコールで高速に描画することができます。

###### Graphics.DrawMeshInstancedIndirect()メソッド
このスクリプトでは、Graphics.DrawMeshInstancedIndirectメソッドを用いてGPUインスタンシングによるメッシュ描画を行います。
このメソッドでは、メッシュのインデックス数やインスタンス数をComputeBufferとして渡すことができます。GPUからすべてのインスタンスデータを読み込みたい場合に便利です。

Start()では、このGPUインスタンシングのための引数バッファを初期化しています。初期化時のコンストラクタの3つ目の引数には*ComputeBufferType.IndirectArguments*を指定します.

RenderInstancedMesh()では、GPUインスタンシングによるメッシュ描画を実行しています。描画のためのマテリアルInstanceRenderMaterialに、SetBufferメソッドで、Boidsシミュレーションによって得られたBoidのデータ（速度、位置の配列）を渡しています。

Graphics.DrawMeshInstancedIndrectメソッドには、インスタンシングするメッシュ、submeshのインデックス、描画用マテリアル、境界データ、また、インスタンス数などのデータを格納したバッファを引数に渡します。

このメソッドは通常Update()内で呼ばれるようにします。

### BoidsRender.shader
Graphics.DrawMeshInstancedIndrectメソッドに対応した描画用のシェーダです。

```

Shader "Hidden/GPUBoids/BoidsRender"
{
    Properties
    {
        _Color ("Color", Color) = (1,1,1,1)
        _MainTex ("Albedo (RGB)", 2D) = "white" {}
        _Glossiness ("Smoothness", Range(0,1)) = 0.5
        _Metallic ("Metallic", Range(0,1)) = 0.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200
        
        CGPROGRAM
        #pragma surface surf Standard vertex:vert addshadow
        #pragma instancing_options procedural:setup
        
        struct Input
        {
            float2 uv_MainTex;
        };
        // Boidの構造体
        struct BoidData
        {
            float3 velocity; // 速度
            float3 position; // 位置
        };

        #ifdef UNITY_PROCEDURAL_INSTANCING_ENABLED
        // Boidデータの構造体バッファ
        StructuredBuffer<BoidData> _BoidDataBuffer;
        #endif

        sampler2D _MainTex; // テクスチャ

        half   _Glossiness; // 光沢
        half   _Metallic;   // 金属特性
        fixed4 _Color;      // カラー

        float3 _ObjectScale; // Boidオブジェクトのスケール

        // オイラー角（ラジアン）を回転行列に変換
        float4x4 eulerAnglesToRotationMatrix(float3 angles)
        {
            float ch = cos(angles.y); float sh = sin(angles.y); // heading
            float ca = cos(angles.z); float sa = sin(angles.z); // attitude
            float cb = cos(angles.x); float sb = sin(angles.x); // bank

            // RyRxRz (Yaw Pitch Roll)
            return float4x4(
                ch * ca + sh * sb * sa, -ch * sa + sh * sb * ca, sh * cb, 0,
                cb * sa, cb * ca, -sb, 0,
                -sh * ca + ch * sb * sa, sh * sa + ch * sb * ca, ch * cb, 0,
                0, 0, 0, 1
            );
        }

        // 頂点シェーダ
        void vert(inout appdata_full v)
        {
            #ifdef UNITY_PROCEDURAL_INSTANCING_ENABLED

            // インスタンスIDからBoidのデータを取得
            BoidData boidData = _BoidDataBuffer[unity_InstanceID]; 

            float3 pos = boidData.position.xyz; // Boidの位置を取得
            float3 scl = _ObjectScale;          // Boidのスケールを取得

            // オブジェクト座標からワールド座標に変換する行列を定義
            float4x4 object2world = (float4x4)0; 
            // スケール値を代入
            object2world._11_22_33_44 = float4(scl.xyz, 1.0);
            // 速度からY軸についての回転を算出
            float rotY = 
                atan2(-boidData.velocity.z, boidData.velocity.x)
                + UNITY_PI * 0.5;
            // 速度からX軸についての回転を算出
            float rotX = 
                -asin(boidData.velocity.y / (length(boidData.velocity.xyz)
                + 1e-8)); // 0除算防止
            // オイラー角（ラジアン）から回転行列を求める
            float4x4 rotMatrix = 
                eulerAnglesToRotationMatrix(float3(rotX, rotY, 0));
            // 行列に回転を適用
            object2world = mul(rotMatrix, object2world);
            // 行列に位置（平行移動）を適用
            object2world._14_24_34 += pos.xyz;

            // 頂点を座標変換
            v.vertex = mul(object2world, v.vertex);
            // 法線を座標変換
            v.normal = normalize(mul(object2world, v.normal));
            #endif
        }
        
        void setup()
        {
        }

        // サーフェスシェーダ
        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb;
            o.Metallic = _Metallic;
            o.Smoothness = _Glossiness;
        }
        ENDCG
    }
    FallBack "Diffuse"
}

```

\#pragma surface surf Standard vertex:vert addshadow
この部分では、サーフェスシェーダとしてsurf()、ライティングモデルはStandard、カスタム頂点シェーダとしてvert()を指定するという処理を行っています。

\#pragma instancing\_options ディレクティブで procedural:FunctionName と指定することによって、Graphics.DrawMeshInstancedIndirectメソッドを使うときのための追加のバリアントを生成するようにUnityに指示することができ、頂点シェーダステージの始めに、FunctionNameで指定した関数が呼ばれるようになります。
公式のサンプル（https://docs.unity3d.com/ScriptReference/
Graphics.DrawMeshInstancedIndirect.html）などを見ると、この関数内で、個々のインスタンスの位置や回転、スケールに基づき、unity\_ObjectToWorld行列, unity\_WorldToObject行列の書き換えを行っていましたが、このサンプルプログラムでは、頂点シェーダ内でBoidsのデータを受け取り、頂点や法線の座標変換を行っています。（良いのかわかりませんが…）
そのため、指定したsetup関数内では何も記述していません。

#### 頂点シェーダでインスタンスごとのBoidのデータを取得し座標変換をする
頂点シェーダ（Vertex Shader）に、シェーダに渡されたメッシュの頂点に対して行う処理を記述します。

unity_InstanceIDによってインスタンスごとに固有のIDを取得することができます。このIDをBoidデータのバッファとして宣言したStructuredBufferの配列のインデックスに指定することによって、インスタンスごとに固有のBoidデータを得ることができます。

#### 回転を求める
Boidの速度データから、進行方向を向くような回転の値を算出します。
ここでは直感的に扱うために、回転はオイラー角で表現することにします。
Boidを飛行体と捉えると、オブジェクトを基準とした座標の3軸の回転は、それぞれ、ピッチ、ヨー、ロールと呼ばれます。

![軸と回転の呼称](images/oishi/roll-pitch-yaw.png)

まず、Z軸についての速度とX軸についての速度から、逆正接（アークタンジェント）を返すatan2メソッドを用いてヨー（水平面に対してどの方向を向いているか）を求めます。

![速度と角度（ヨー）の関係](images/oishi/arctan.png)

次に、速度の大きさと、Y軸についての速度の比率から、逆正弦（アークサイン）を返すasinメソッドを用いてピッチ（上下の傾き）を求めています。それぞれの軸についての速度の中でY軸の速度が小さい場合は、変化が少なく水平を保つように重みのついた回転量になるようになっています。

![速度と角度（ピッチ）の関係](images/oishi/arcsin.png)

#### Boidのトランスフォームを適用する行列を計算
移動、回転、拡大縮小といった座標変換処理は、まとめて一つの行列で表現することができます。
4x4の行列object2worldを定義します。

##### 拡大縮小
まず、スケール値を代入します。
XYZ軸それぞれに @<m>{\rm S\_x S\_y S\_z {\\}} だけ拡大縮小を行う行列Sは以下のように表現されます。

//texequation{
\rm
S=
\left(
\begin{array}{cccc}
\rm S\_x & 0 & 0 & 0 \\\
0 & \rm S\_y & 0 & 0 \\\
0 & 0 & \rm S\_z & 0 \\\
0 & 0 & 0 & 1
\end{array}
\right)
//}

HLSLのfloat4x4型の変数は、.\_11\_22\_33\_44のようなスィズルを用いて行列の特定の要素を指定できます。
デフォルトであれば、成分は以下のように整列してます。

|11|12|13|14|
|:--:|:--:|:--:|:--:|
|21|22|23|24|
|31|32|33|34|
|41|42|43|44|

ここでは、11、22、33、にXYZそれぞれのスケールの値、44には1を代入します。

##### 回転
次に、回転を適用します。
XYZ軸それぞれについての回転 @<m>{\rm R\_x R\_y R\_z {\\}} を行列で表現すると、

//texequation{
\rm
R_x(\phi)=
\left(
\begin{array}{cccc}
1 & 0 & 0 & 0 \\\
0 & \rm cos(\phi) & \rm -sin(\phi) & 0 \\\
0 & \rm sin(\phi) & \rm cos(\phi) & 0 \\\
0 & 0 & 0 & 1
\end{array}
\right)
//}

//texequation{
\rm
R_y(\theta)=
\left(
\begin{array}{cccc}
\rm cos(\theta) & 0 & \rm sin(\theta) & 0 \\\
0 & 1 & 0 & 0 \\\
\rm -sin(\theta) & 0 & \rm cos(\theta) & 0 \\\
0 & 0 & 0 & 1
\end{array}
\right)
//}

//texequation{
\rm
R_z(\psi)=
\left(
\begin{array}{cccc}
\rm cos(\psi) & \rm -sin(\psi) & 0 & 0 \\\
\rm sin(\psi) & \rm cos(\psi) & 0 & 0 \\\
0 & 0 & 1 & 0 \\\
0 & 0 & 0 & 1
\end{array}
\right)
//}

これを一つに行列に合成します。このとき、合成する回転の軸の順で回転時の挙動が変化しますが、この順に合成すると、Unityの標準の回転と同様のものになるはずです。

![回転行列の合成](images/oishi/synth-euler2matrix.png)

これによって求められた回転行列と、スケールを適用した行列との積を求めることによって、回転を適用します。

##### 平行移動
次に、平行移動を適用します。
それぞれの軸に、 @<m>{\rm T\_x T\_y T\_z {\\}} 平行移動するとすると、行列は以下のように表現されます。

//texequation{
\rm T=
\left(
\begin{array}{cccc}
1 & 0 & 0 & \rm T\_x \\\
0 & 1 & 0 & \rm T\_y \\\
0 & 0 & 1 & \rm T\_z \\\
0 & 0 & 0 & 1
\end{array}
\right)
//}

この平行移動は、14, 24, 34成分にXYZそれぞれの軸についての位置（Position）データを加算することで適用できます。

これらの計算によって得られた行列を、頂点、法線に適用させることによって、Boidのトランスフォームデータを反映します。

### 描画結果
このように群れっぽい動きをするオブジェクトが描画されると思います。

![実行結果](images/oishi/result.png)

## まとめ
この章で紹介した実装は、最低限のBoidsのアルゴリズムを利用したものですが、パラメータの調整によっても、群は様々異なる特徴を持った動きを見せると思います。加えて、ここで示したものの他に、自然界には多く運動のルールが存在します。例えば、これが魚の群だとして、それらを捕食する外敵が現れたとすると当然逃げるような動きをし、地形など障害物があるとすれば魚はぶつからないように避けるでしょう。視覚について考えると、動物の種によっては視野や精度も異なり、視界の外の他の個体は計算処理から除外するなどすると、より実際のものに近づいていくと思います。空を飛ぶのか、水の中を動くのか、陸上を移動するのかといった環境や、移動運動のための運動器官の特性によっても動きの特徴が変わってきます。個体差にも着眼すべきです。

GPUによる並列処理は、CPUによる演算に比べれば多くの個体を計算できますが、基本的には他の個体との計算は総当たりで行っており、計算効率はあまり良いとは言えません。それには、個体をその位置によってグリッドやブロックで分割した領域に登録しておき、隣接した領域に存在する個体についてだけ計算処理を行うというように、近傍個体探索の効率化を図ることで計算コストを抑えることができます。
このように改良の余地は多く残されており、適切な実装と行動のルールを適用することにより、いっそう美しく、迫力、密度と味わいのある群の動きが表現できるようになることと思います。できるようになりたいです。


## 参照

* Boids Background and Update - https://www.red3d.com/cwr/boids/
* THE NATURE OF CODE - http://natureofcode.com/
* Real-Time Particle Systems on the GPU in Dynamic Environments

- http://amd-dev.wpengine.netdna-cdn.com/wordpress/media/2013/02/Chapter7-Drone-Real-Time_Particle_Systems_On_The_GPU.pdf

* Practical Rendering and Computation with Direct3D 11

- https://dl.acm.org/citation.cfm?id=2050039

* GPU 並列図形処理入門 - http://gihyo.jp/book/2014/978-4-7741-6304-8











