= ComputeShader 入門


Unity で ComputeShader (以降必要に応じて"コンピュートシェーダ") を使う方法について、シンプルに解説します。
コンピュートシェーダとは、GPU を使って単純処理を並列化し、大量の演算を高速に実行するために用いられます。
また GPU に処理を委譲しますが、通常のレンダリングパイプラインとは異なることが特徴に挙げられます。
CG においては、大量のパーティクルの動きを表現するためなどに良く用いられます。

本章の以降に続く内容の一部にも、コンピュートシェーダが用いられたものがあり、
それらを読み進める上で、コンピュートシェーダの知識が必要になります。

ここではコンピュートシェーダを学習するにあたって、
一番最初の足掛かりになるような内容について、2 つの簡単なサンプルを用いて解説しています。
これらはコンピュートシェーダのすべての事について扱うものではありませんので、必要に応じて情報を補うようにしてください。

Unity においては ComputeShader と呼称していますが、
類似する技術に OpenCL, DirectCompute, CUDA などが挙げられます。
基本概念は類似しており、特に DirectCompute(DirectX) と非常に近い関係にあります。
もしアーキテクチャ周辺の概念や更なる詳細情報が必要になるときは、
これらについても合わせて情報を集めるようにすると良いと思います。

本章のサンプルは @<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming} の「SimpleComputeShader」です。


== カーネル、スレッド、グループの概念


//image[primerofcomputeshader01][カーネル、スレッド、グループのイメージ][scale=1]

具体的な実装を解説するよりも前に、コンピュートシェーダで取り扱われる @<b>{カーネル(Kernel)}、
@<b>{スレッド(Thread)}、@<b>{グループ(Group)} の概念を説明しておく必要があります。


@<b>{カーネル} とは、GPU で実行される 1 つの処理を指し、
コード上では 1 つの関数として扱われます(一般的なシステム用語における意味でのカーネルに相当)。

@<b>{スレッド}とは、カーネルを実行する単位です。1 スレッドが、1 カーネルを実行します。
コンピュートシェーダではカーネルを複数のスレッドで並行して同時に実行することができます。
スレッドは (x, y, z) の3次元で指定しす。

例えば、(4, 1, 1) なら 4 * 1 * 1 = 4 つのスレッドが同時に実行されます。
(2, 2, 1) なら、2 * 2 * 1 = 4 つのスレッドが同時に実行されます。
同じ 4 つのスレッドが実行されますが、状況に応じて、後者のような 2 次元でスレッドを指定する方が効率が良いことがあります。
これについては後に続いて解説します。ひとまずスレッド数は 3 次元で指定されるという認識が必要です。

最後に@<b>{グループ}とは、スレッドを実行する単位です。また、あるグループが実行するスレッドは@<b>{グループスレッド}と呼ばれます。
例えば、あるグループが単位当たり、(4, 1, 1) スレッドを持つとします。このグループが 2 つあるとき、それぞれのグループが、(4, 1, 1) のスレッドを持ちます。

グループもスレッドと同様に 3 次元で指定されます。例えば、(2, 1, 1) グループが、(4, 4, 1) スレッドで実行されるカーネルを実行するとき、
グループ数は 2 * 1 * 1 = 2 です。この 2 つのグループは、それぞれ 4 * 4 * 1 = 16 スレッドを持つことになります。
したがって、合計スレッド数は、2 * 16 = 32 となります。


== サンプル (1) : GPU で演算した結果を取得する


サンプル (1) 「SampleScene_Array」では、適当な計算をコンピュートシェーダで実行し、その結果を配列として取得する方法について扱います。
サンプルには次のような操作が含まれます。

 * コンピュートシェーダを使って複数のデータを処理し、その結果を取得する。
 * コンピュートシェーダに複数の機能を実装し、使い分ける。
 * コンピュートシェーダ (GPU) にスクリプト (CPU) から値を渡す。

サンプル (1) の実行結果は次の通りになります。デバッグ出力だけですから、ソースコードを読みながら動作を確認してください。

//image[primerofcomputeshader03][サンプル (1) の実行結果][scale=1]


=== コンピュートシェーダの実装


ここからサンプルを実例に解説を進めます。
非常に短いので、コンピュートシェーダの実装については先に一通り目を通して頂くのが良いと思います。
基本構成として、関数の定義、関数の実装、バッファがあり、必要に応じて変数があります。

//emlist[SimpleComputeShader_Array.compute]{
#pragma kernel KernelFunction_A
#pragma kernel KernelFunction_B

RWStructuredBuffer<int> intBuffer;
float floatValue;

[numthreads(4, 1, 1)]
void KernelFunction_A(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] = groupThreadID.x * floatValue;
}

[numthreads(4, 1, 1)]
void KernelFunction_B(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] += 1;
}
//}

特徴として、@<b>{numthreads} 属性と、@<b>{SV_GroupID} セマンティクスなどがありますが、
これについては後述します。


=== カーネルの定義


先に解説した通り、正確な定義はさておき、@<b>{カーネルは GPU で実行される1つの処理を指し、コード上では 1 つの関数として扱われます。}
カーネルは 1 つのコンピュートシェーダに複数実装することができます。

この例では、カーネルは @<code>{KernelFunction_A} ないし @<code>{KernelFunction_B} 関数がカーネルに相当します。
また、カーネルとして扱う関数は @<code>{#pragma kernel} を使って定義します。
これによってカーネルとそれ以外の関数と識別します。

定義された複数のカーネルのうち、任意の 1 つを識別するために、固有のインデックスがカーネルに与えられます。
インデックスは @<code>{#pragma kernel} で定義された順に、上から 0, 1 … と与えられます。


=== バッファや変数の用意


コンピュートシェーダで実行した結果を保存する@<b>{バッファ領域}を作っておきます。
サンプルの変数 @<code>{RWStructuredBuffer<int> intBuffer}} がこれに相当します。

またスクリプト (CPU) 側から任意の値を与えたい場合には、一般的な CPU プログラミングと同じように変数を用意します。
この例では変数 @<code>{intValue} がこれに相当し、スクリプトから値を渡します。


=== numthreads による実行スレッド数の指定


@<b>{numthreads} 属性 (Attribute) は、カーネル (関数) を実行するスレッドの数を指定します。
スレッド数の指定は、(x, y, z) で指定し、例えば (4, 1, 1) なら、 4 * 1 * 1 = 4 スレッドでカーネルを実行します。
他に、(2, 2, 1) なら 2 * 2 * 1 = 4 スレッドでカーネルを実行します。
共に 4 スレッドで実行されますが、この違いや使い分けについては後述します。


=== カーネル (関数) の引数


カーネルに設定できる引数には制約があり、一般的な CPU プログラミングと比較して自由度は極めて低いです。

引数に続く値を@<code>{セマンティクス}と呼び、この例では @<code>{groupID : SV_GroupID} と @<code>{groupThreadID : SV_GroupThreadID} を設定しています。セマンティクスは引数がどのような値であるかを示すための物であり、他の名前に変更することができません。

引数名 (変数名) は自由に定義することができますが、コンピュートシェーダを使うにあたって定義されるセマンティクスのいずれかを設定する必要があります。
つまり、任意の型の引数を定義してカーネル内で参照する、といった実装はできず、
カーネルで参照することができる引数は、定められた限定的なものから選択する、ということです。

@<code>{SV_GroupID} は、カーネルを実行するスレッドが、どのグループで実行されているかを (x, y, z) で示します。
@<code>{SV_GroupThreadID} は、カーネルを実行するスレッドが、グループ内の何番目のスレッドであるかを (x, y, z) で示します。

例えば (4, 4, 1) のグループで、(2, 2, 1) のスレッドを実行するとき、
@<code>{SV_GroupID} は (0 ~ 3, 0 ~ 3, 0) の値を返します。
@<code>{SV_GroupThreadID} は (0 ~ 1, 0 ~ 1, 0) の値を返します。

サンプルで設定されるセマンティクス以外にも @<code>{SV_~} から始まるセマンティクスがあり、利用することができますが、
ここでは説明を割愛します。一通りコンピュートシェーダの動きが分かった後に目を通すほうが良いと思います。

 * SV_GroupID - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/ja-jp/library/ee422449(v=vs.85).aspx} 
 ** 異なる SV~ セマンティクスとその値について確認することができます。


=== カーネル (関数) の処理内容


サンプルでは、用意したバッファに、順にスレッド番号を代入していく処理を行っています。
@<code>{groupThreadID} は、あるグループで実行されるスレッド番号が与えられます。
このカーネルは (4, 1, 1) スレッドで実行されますから、@<code>{groupThreadID} は (0 ~ 3, 0, 0) が与えられます。

//emlist[SimpleComputeShader_Array.compute]{
[numthreads(4, 1, 1)]
void KernelFunction_A(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] = groupThreadID.x * intValue;
}
//}

今回のサンプルはこのスレッドを、(1, 1, 1) のグループで実行します (後述するスクリプトから) 。
すなわちグループを 1 つだけ実行し、そのグループには、4 * 1 * 1 のスレッドが含まれます。
結果として@<code>{groupThreadID.x} には 0 ~ 3 の値が与えられることを確認してください。

※この例では @<code>{groupID} を利用していませんが、スレッドと同様に、3次元で指定されるグループ数が与えられます。
代入してみるなどして、コンピュートシェーダの動きを確認するために使ってみてください。


=== スクリプトからコンピュートシェーダを実行する


実装したコンピュートシェーダをスクリプトから実行します。スクリプト側で必要になるものは概ね次の通りです。

 * コンピュートシェーダへの参照 | @<code>{comuteShader}
 * 実行するカーネルのインデックス | @<code>{kernelIndex_KernelFunction_A, B}
 * コンピュートシェーダの実行結果を保存するバッファ | @<code>{intComputeBuffer}

//emlist[SimpleComputeShader_Array.cs]{
public ComputeShader computeShader;
int kernelIndex_KernelFunction_A;
int kernelIndex_KernelFunction_B;
ComputeBuffer intComputeBuffer;

void Start()
{
    this.kernelIndex_KernelFunction_A
        = this.computeShader.FindKernel("KernelFunction_A");
    this.kernelIndex_KernelFunction_B
        = this.computeShader.FindKernel("KernelFunction_B");

    this.intComputeBuffer = new ComputeBuffer(4, sizeof(int));
    this.computeShader.SetBuffer
        (this.kernelIndex_KernelFunction_A,
         "intBuffer", this.intComputeBuffer);

    this.computeShader.SetInt("intValue", 1);
    …
//}


=== 実行するカーネルのインデックスを取得する


あるカーネルを実行するためには、そのカーネルを指定するためのインデックス情報が必要です。
インデックスは @<code>{#pragma kernel} で定義された順に、上から 0, 1 … と与えられますが、
スクリプト側から @<code>{FindKernel} 関数を使うのが良いでしょう。

//emlist[SimpleComputeShader_Array.cs]{
this.kernelIndex_KernelFunction_A
    = this.computeShader.FindKernel("KernelFunction_A");

this.kernelIndex_KernelFunction_B
    = this.computeShader.FindKernel("KernelFunction_B");
//}


=== 演算結果を保存するバッファの生成する


コンピュートシェーダ (GPU) による演算結果を CPU 側に保存するためのバッファ領域を用意します。
Unity　では @<code>{ComputeBuffer} として定義されています。

//emlist[SimpleComputeShader_Array.cs]{
this.intComputeBuffer = new ComputeBuffer(4, sizeof(int));
this.computeShader.SetBuffer
    (this.kernelIndex_KernelFunction_A, "intBuffer", this.intComputeBuffer);
//}

@<code>{ComputeBuffer} を (1) 保存する領域のサイズ、
(2) 保存するデータの単位当たりのサイズを指定して初期化します。
ここでは int 型のサイズ 4 つ分の領域が用意されています。
これはコンピュートシェーダの実行結果が int[4] として保存されるためです。
必要に応じてサイズを変更します。

次いで、コンピュートシェーダに実装された、(1) どのカーネルが実行するときに、
(2) どの GPU 上のバッファを使うのかを指定し、(3) CPU 上のどのバッファに相当するのか、を指定します。

この例では、(1) @<code>{KernelFunction_A} が実行されるときに参照される、
(2) @<code>{intBuffer} なるバッファ領域は、(3) @<code>{intComputeBuffer} に相当する、と指定されます。


=== スクリプトからコンピュートシェーダに値を渡す


//emlist[SimpleComputeShader_Array.cs]{
this.computeShader.SetInt("intValue", 1);
//}

処理したい内容によってはスクリプト (CPU) 側からコンピュートシェーダ (GPU) 側に値を渡し、参照したい場合があると思います。
ほとんどの型の値は @<code>{ComputeShader.Set~} を使って、コンピュートシェーダ内にある変数に設定することができます。
このとき、引数に設定する引数の変数名と、コンピュートシェーダ内に定義された変数名は一致する必要があります。
この例では @<code>{intValue} に 1 を渡しています。


=== コンピュートシェーダの実行


コンピュートシェーダに実装(定義)されたカーネルは、@<code>{ComputeShader.Dispatch} メソッドで実行します。
指定したインデックスのカーネルを、指定したグループ数で実行します。グループ数は X * Y * Z で指定します。このサンプルでは 1 * 1 * 1 = 1 グループです。

//emlist[SimpleComputeShader_Array.cs]{
this.computeShader.Dispatch
    (this.kernelIndex_KernelFunction_A, 1, 1, 1);

int[] result = new int[4];

this.intComputeBuffer.GetData(result);

for (int i = 0; i < 4; i++)
{
    Debug.Log(result[i]);
}
//}

コンピュートシェーダ (カーネル) の実行結果は、@<code>{ComputeBuffer.GetData} で取得します。


=== 実行結果の確認 (A)


あらためてコンピュートシェーダ側の実装を確認します。
このサンプルでは次のカーネルを 1 * 1 * 1 = 1グループで実行しています。
スレッドは、4 * 1 * 1 = 4 スレッドです。また @<code>{intValue} にはスクリプトから 1 を与えています。

//emlist[SimpleComputeShader_Array.compute]{
[numthreads(4, 1, 1)]
void KernelFunction_A(uint3 groupID : SV_GroupID,
                      uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] = groupThreadID.x * intValue;
}
//}


@<code>{groupThreadID(SV_GroupThreadID)} は、
今このカーネルが、グループ内の何番目のスレッドで実行されているかを示す値が入るので、
この例では (0 ~ 3, 0, 0) が入ります。したがって、@<code>{groupThreadID.x} は 0 ~ 3 です。
つまり、@<code>{intBuffer[0] = 0}　～ @<code>{intBuffer[3] = 3} までが並列して実行されることになります。


=== 異なるカーネル (B) を実行する


1 つのコンピュートシェーダに実装した異なるカーネルを実行するときは、別のカーネルのインデックスを指定します。
この例では、@<code>{KernelFunction_A} を実行した後に @<code>{KernelFunction_B} を実行します。
さらに @<code>{KernelFunction_A} で利用したバッファ領域を、@<code>{KernelFunction_B} でも使っています。

//emlist[SimpleComputeShader_Array.cs]{
this.computeShader.SetBuffer
(this.kernelIndex_KernelFunction_B, "intBuffer", this.intComputeBuffer);

this.computeShader.Dispatch(this.kernelIndex_KernelFunction_B, 1, 1, 1);

this.intComputeBuffer.GetData(result);

for (int i = 0; i < 4; i++)
{
    Debug.Log(result[i]);
}
//}


=== 実行結果の確認 (B)


@<code>{KernelFunction_B} は次のようなコードを実行します。
このとき @<code>{intBuffer} は @<code>{KernelFunction_A} で使ったものを引き続き指定している点に注意してください。

//emlist[SimpleComputeShader_Array.compute]{
RWStructuredBuffer<int> intBuffer;

[numthreads(4, 1, 1)]
void KernelFunction_B
(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    intBuffer[groupThreadID.x] += 1;
}
//}

このサンプルでは、@<code>{KernelFunction_A} によって @<code>{intBuffer} に 0 ~ 3 が順に与えられています。
したがって @<code>{KernelFunction_B} を実行した後は、値が 1 ~ 4 になることを確認します。


=== バッファの破棄


利用し終えた ComputeBuffer は、明示的に破棄する必要があります。

//emlist[SimpleComputeShader_Array.cs]{
this.intComputeBuffer.Release();
//}


=== サンプル (1) で解決していない問題


多次元のスレッドまたはグループを指定する意図について、このサンプルでは解説していません。
例えば、 (4, 1, 1) スレッドと、(2, 2, 1) スレッドは、どちらも 4 スレッド実行されますが、
この 2 つは使い分ける意味があります。これについては後に続くサンプル (2) で解説します。


== サンプル (2) : GPU の演算結果をテクスチャにする


サンプル (2) 「SampleScene_Texture」では、コンピュートシェーダの算出結果をテクスチャにして取得します。
サンプルには次のような操作が含まれます。

 * コンピュートシェーダを使って、テクスチャに情報を書き込む。
 * 多次元 (2次元) のスレッドを有効に活用する。

サンプル (2) の実行結果は次の通りになります。横方向と縦方向にグラデーションするテクスチャを生成します。

//image[primerofcomputeshader04][サンプル (2) の実行結果][scale=1]


=== カーネルの実装


全体の実装についてはサンプルを参照してください。このサンプルでは概ね次のようなコードをコンピュートシェーダで実行します。
カーネルが多次元スレッドで実行される点に注目してください。(8, 8, 1) なので、1 グループあたり、8 * 8 * 1 = 64 スレッドで実行されます。
また演算結果の保存先が @<code>{RWTexture2D<float4>} であることも大きな変更点です。

//emlist[SimpleComputeShader_Texture.compute]{
RWTexture2D<float4> textureBuffer;

[numthreads(8, 8, 1)]
void KernelFunction_A(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float width, height;
    textureBuffer.GetDimensions(width, height);

    textureBuffer[dispatchThreadID.xy]
        = float4(dispatchThreadID.x / width,
                 dispatchThreadID.x / width,
                 dispatchThreadID.x / width,
                 1);
}
//}


=== 特殊な引数 SV_DispatchThreadID


サンプル (1) では @<code>{SV_DispatchThradID} セマンティクスは使いませんでした。
少々複雑ですが、@<b>{「あるカーネルを実行するスレッドが、すべてのスレッドの中のどこに位置するか (x,y,z) 」}を示しています。

@<code>{SV_DispathThreadID} は、@<code>{SV_Group_ID * numthreads + SV_GroupThreadID} で算出される値です。
@<code>{SV_Group_ID} はあるグループを (x, y, z) で示し、@<code>{SV_GroupThreadID} は、あるグループに含まれるスレッドを (x, y, z) で示します。


==== 具体的な計算例 (1)


例えば、(2, 2, 1) グループで、(4, 1, 1) スレッドで実行される、カーネルを実行するとします。
その内の 1 つのカーネルは、(0, 1, 0) 番目のグループに含まれる、(2, 0, 0) 番目のスレッドで実行されます。
このとき @<code>{SV_DispatchThreadID} は、(0, 1, 0) * (4, 1, 1) + (2, 0, 0) = (0, 1, 0) + (2, 0, 0) = (2, 1, 0) になります。


==== 具体的な計算例 (2)


今度は最大値を考えましょう。(2, 2, 1) グループで、(4, 1, 1) スレッドでカーネルが実行されるとき、
(1, 1, 0) 番目のグループに含まれる、(3, 0, 0) 番目のスレッドが最後のスレッドです。
このとき @<code>{SV_DispatchThreadID} は、(1, 1, 0) * (4, 1, 1) + (3, 0, 0) = (4, 1, 0) + (3, 0, 0) = (7, 1, 0) になります。


=== テクスチャ (ピクセル) に値を書き込む


以降は時系列順に解説するのが困難ですので、サンプル全体に目を通しながら確認してください。

サンプル (2) の @<code>{dispatchThreadID.xy} は、
テクスチャ上にあるすべてのピクセルを示すように、グループとスレッドを設定しています。
グループを設定するのはスクリプト側なので、スクリプトとコンピュートシェーダを横断して確認する必要があります。

//emlist[SimpleComputeShader_Texture.compute]{
textureBuffer[dispatchThreadID.xy]
    = float4(dispatchThreadID.x / width,
             dispatchThreadID.x / width,
             dispatchThreadID.x / width,
             1);
//}

このサンプルでは仮に 512x512 のテクスチャを用意していますが、@<code>{dispatchThreadID.x} が 0 ~ 511 を示すとき、
@<code>{dispatchThreadID / width} は、0 ~ 0.998… を示します。
つまり @<code>{dispatchThreadID.xy} の値( = ピクセル座標)が大きくなるにつれて、黒から白に塗りつぶしていくことになります。

//note{
テクスチャは、RGBA チャネルから構成され、それぞれ 0 ~ 1 で設定します。
すべて 0 のとき、完全に黒くなり、すべて 1 のとき、完全に白くなります。
//}


=== テクスチャの用意


以降がスクリプト側の実装の解説です。サンプル (1) では、コンピュートシェーダの計算結果を保存するために配列のバッファを用意しました。
サンプル (2) では、その代わりにテクスチャを用意します。

//emlist[SimpleComputeShader_Texture.cs]{
RenderTexture renderTexture_A;
…
void Start()
{
    this.renderTexture_A = new RenderTexture
        (512, 512, 0, RenderTextureFormat.ARGB32);
    this.renderTexture_A.enableRandomWrite = true;
    this.renderTexture_A.Create();
…
//}

解像度とフォーマットを指定して RenderTexture を初期化します。
このとき @<code>{RenderTexture.enableRandomWrite} を有効にして、
テクスチャへの書き込みを有効にしている点に注意します。

 * RenderTexture.enableRandomWrite - Unity
 ** @<href>{https://docs.unity3d.com/ScriptReference/RenderTexture-enableRandomWrite.html}


=== スレッド数の取得


カーネルのインデックスが取得できるように、カーネルがどれくらいのスレッド数で実行できるかも取得できます(スレッドサイズ)。

//emlist[SimpleComputeShader_Texture.cs]{
void Start()
{
…
    uint threadSizeX, threadSizeY, threadSizeZ;

    this.computeShader.GetKernelThreadGroupSizes
     (this.kernelIndex_KernelFunction_A,
      out threadSizeX, out threadSizeY, out threadSizeZ);
…
//}


=== カーネルの実行


@<code>{Dispath} メソッドで処理を実行します。このとき、グループ数の指定方法に注意します。
この例では、グループ数は「テクスチャの水平(垂直)方向の解像度 / 水平(垂直)方向のスレッド数」で算出しています。

水平方向について考えるとき、テクスチャの解像度は 512、スレッド数は 8 ですから、
水平方向のグループ数は 512 / 8 = 64 になります。同様に垂直方向も 64 です。
したがって、合計グループ数は 64 * 64 = 4096 になります。

//emlist[SimpleComputeShader_Texture.cs]{
void Update()
{
    this.computeShader.Dispatch
    (this.kernelIndex_KernelFunction_A,
     this.renderTexture_A.width  / this.kernelThreadSize_KernelFunction_A.x,
     this.renderTexture_A.height / this.kernelThreadSize_KernelFunction_A.y,
     this.kernelThreadSize_KernelFunction_A.z);

    plane_A.GetComponent<Renderer>()
        .material.mainTexture = this.renderTexture_A;
//}

言い換えれば、各グループは 8 * 8 * 1 = 64 (= スレッド数) ピクセルずつ処理することになります。
グループは 4096 あるので、4096 * 64 = 262,144 ピクセル処理します。
画像は、512 * 512 = 262,144 ピクセルなので、ちょうどすべてのピクセルを並列に処理できたことになります。


==== 異なるカーネルの実行


もう一方のカーネルは、x ではなく、 y 座標を使って塗りつぶしていきます。
このとき 0 に近い値、黒い色が下のほうに表れている点に注意します。
テクスチャを操作するときは原点を考慮しなければならないこともあります。


=== 多次元スレッド、グループの利点


サンプル (2) のように、多次元の結果が必要な場合、あるいは多次元の演算が必要な場合には、多次元のスレッドやグループが有効に働きます。
もしサンプル (2) を 1 次元のスレッドで処理しようとすると、縦方向のピクセル座標を任意に算出する必要があります。

//note{
実際に実装しようとすると確認できますが、画像処理でいうところのストライド、例えば 512x512 の画像があるとき、
その 513 番目のピクセルは、(0, 1) 座標になる、といった算出が必要になります。
//}

演算数は削減したほうが良いですし、高度な処理を行うにしたがって複雑さは増します。
コンピュートシェーダを使った処理を設計するときは、上手く多次元を活用できないか検討するのが良いです。


== さらなる学習のための補足情報


本章ではコンピュートシェーダについてサンプルを解説する形式で入門情報としましたが、
これから先、学習を進める上で必要ないくつかの情報を補足します。


=== GPU アーキテクチャ・基本構造


//image[primerofcomputeshader02][GPU アーキテクチャのイメージ][scale=1]

GPU のアーキテクチャ・構造についての基本的な知識があれば、
コンピュートシェーダを使った処理の実装の際、それを最適化するために役に立つので、少しだけここで紹介します。

GPU は多数の @<b>{Streaming Multiprocessor(SM)} が搭載されていて、そ
れらが分担・並列化して与えられた処理を実行します。

SM には更に小さな @<b>{Streaming Processor(SP)} が複数搭載されていて、
SM に割り当てられた処理を SP が計算する、といった形式です。

SM には@<b>{レジスタ}と@<b>{シェアードメモリ}が搭載されていて、
@<b>{グローバルメモリ(DRAM上のメモリ)}よりも高速に読み書きすることができます。
レジスタは関数内でのみ参照されるローカル変数に使われ、
シェアードメモリは同一 SM 内に所属するすべての SP から参照し書き込むことができます。

つまり、各メモリの最大サイズやスコープを把握し、
無駄なく高速にメモリを読み書きできる最適な実装を実現できるのが理想です。

例えば最も考慮する必要があるであろうシェアードメモリは、
クラス修飾子 (storage-class modifiers) @<code>{groupshared} を使って定義します。
ここでは入門なので具体的な導入例を割愛しますが、最適化に必要な技術・用語として覚えておいて、以降の学習に役立ててください。

 * Variable Syntax - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/en-us/library/bb509706(v=vs.85).aspx}


==== レジスタ


SP に最も近い位置に置かれ、最も高速にアクセスできるメモリ領域です。
4 byte 単位で構成され、カーネル(関数)スコープの変数が配置されます。
スレッドごとに独立するため共有することができません。


==== シェアードメモリ


SM に置かれるメモリ領域で、L1 キャッシュと合わせて管理されています。
同じ SM 内にある SP(= スレッド) で共有することができ、かつ十分に高速にアクセスすることができます。


==== グローバルメモリ


GPU 上ではなく DRAM 上のメモリ領域です。
GPU 上にのプロセッサからは離れた位置にあるため参照は低速です。
一方で、容量が大きく、すべてのスレッドからデータの読み書きが可能です。


==== ローカルメモリ


GPU 上ではなく DRAM 上のメモリ領域で、レジスタに収まらないデータを格納します。
GPU 上のプロセッサからは離れた位置にあるため参照は低速です。


==== テクスチャメモリ


テクスチャデータ専用のメモリで、グローバルメモリをテクスチャ専用に扱います。


==== コンスタントメモリ


読み込み専用のメモリで、カーネル(関数)の引数や定数を保存しておくためなどに使われます。
専用のキャッシュを持っていて、グローバルメモリよりも高速に参照できます。


=== 効率の良いスレッド数指定のヒント


総スレッド数が実際に処理したいデータ数よりも大きい場合は、
無意味に実行される (あるいは処理されない) スレッドが生じることになり非効率です。
総スレッド数は可能な限り処理したいデータ数と一致させるように設計します。


=== 現行スペック上の限界


執筆時時点での現行スペックの上限を紹介します。最新版でない可能性があることに十分に注意してください。
ただし、これらのような制限を考慮しつつ実装することが求められます。

 * Compute Shader Overview - Microsoft Developer Network
 ** @<href>{https://msdn.microsoft.com/en-us/library/ff476331(v=vs.85).aspx}


==== スレッドとグループ数


スレッド数やグループ数の限界については、解説中に言及しませんでした。
これはシェーダモデル(バージョン)によって変更されるためです。
今後も並列できる数は増えていくものと思われます。

 * ShaderModel cs_4_x
 ** Z の最大値が 1
 ** X * Y * Z の最大値が 768

 * ShaderModel cs_5_0
 ** Z の最大値が 64
 ** X * Y * Z の最大値は 1024

またグループの限界は (x, y, z) でそれぞれ 65535 です。


==== メモリ領域


シェアードメモリの上限は、単位グループあたり 16 KB, 
あるスレッドが書き込めるシェアードメモリのサイズは、単位あたり 256 byte までと制限されています。


== 参考


本章でのその他の参考は以下の通りです。

 * 第５回　GPUの構造 - 日本GPUコンピューティングパートナーシップ - @<href>{http://www.gdep.jp/page/view/252}
 * Windows で始める CUDA 入門 - エヌビディアジャパン - @<href>{http://on-demand.gputechconf.com/gtc/2013/jp/sessions/8001.pdf}