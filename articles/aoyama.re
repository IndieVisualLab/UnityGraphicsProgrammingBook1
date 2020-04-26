
= ジオメトリシェーダーで草を生やす

== はじめに

本章ではレンダリングパイプラインのステージの一つであるGeometry Shader(ジオメトリシェーダー)についての説明を主軸として、Geometry Shaderを用いた動的な草生成シェーダー(俗に言うGrass Shader)を解説しています。

Geometry Shaderの説明についてはいくらかの専門的用語を用いていますが、とりあえずGeometry Shaderを使ってみければサンプルコードを見て頂くのが手っ取り早いでしょう。

本章のUnityプロジェクトは以下のGithubリポジトリにアップロードしてあります。

@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming/, https://github.com/IndieVisualLab/UnityGraphicsProgramming/}

== Geometry Shaderとは？

Geometry Shaderとは、GPU上で動的にプリミティブ(メッシュを構成する基本形状)の変換・生成・削除などが可能なプログラマブルシェーダーの一つです。

これまでプリミティブを変換するなど、動的にメッシュ形状を変化させようとすると、CPU上で処理を行うか、事前に頂点にメタ情報を持たせておきVertex Shaderで変換するなどの工夫が必要でした。
しかし、Vertex Shaderでは隣接する頂点に関する情報を取得することが出来ず、処理中の頂点を元に新しく頂点を生成したり、また逆に削除したりする事が出来ないなどの強い制約がありました。
また、だからといってCPUで処理を行うと、リアルタイム処理という観点からすると非現実的なほど膨大な時間を要することになります。
この様に、リアルタイムにメッシュを形状変化させることに関しては、今までいくつかの問題を抱えていました。

そこで、これらの問題を解決し、弱い制約の中で自由に変換処理を出来るようにするための機能として、DirectX10やOpenGL3.2にて標準搭載されたのがGeometry Shaderです。
なお、OpenGLではPrimitive Shaderとも呼ばれることがあります。

== Geometry Shaderの特徴

=== レンダリングパイプライン

レンダリングパイプライン上ではVertex Shaderの次、Fragment Shaderやラスタライズ処理の前に位置しています。
つまり、Fragment Shader内では、Geometry Shaderにて動的に生成した頂点とVertex Shaderに渡された元々の頂点とを区別せずに処理されます。

=== Geometry Shaderへの入力

通常Vertex Shaderへの入力情報は頂点単位となっており、その頂点についての変換処理を行います。ですが、Geometry Shaderへの入力情報はユーザによって定義された入力用プリミティブ単位となります。

実際のプログラムは後述してありますが、Vertex Shaderにて処理をした頂点情報群が、入力用プリミティブ型に基いて分割して入力されることになります。
例えば入力のプリミティブ型をtriangleとすれば3つの頂点情報が、lineとすれば2つの頂点情報が、pointとすれば1つの頂点情報が渡されます。
これによってvertex shaderでは出来なかった、他の頂点情報を参照しながら処理を行なう事が可能となり、幅広い計算が出来るようになります。

なお一つ注意が必要な点として、Vertex Shaderは頂点単位で処理が行われ、その処理する頂点についての情報が渡されますが、Geometry Shaderは入力用プリミティブ型とは関係なく、プリミティブアセンブリのトポロジによって決定されるプリミティブを単位として処理が行われます。
つまり、図6.1のようにトポロジがTrianglesのQuadメッシュにGeometry Shaderを実行する場合、Geometry Shaderは三角形①と②について計2回実行されます。
この時、入力用プリミティブ型をLineとした場合、入力に渡される情報は三角形①の時は頂点0,1,2のうちの二点の頂点、②の時は頂点0,2,3のうちの二点の頂点となります。

//image[aoyama/img0][Quadメッシュ][scale=0.4]


=== Geometry Shaderからの出力


Geometry Shaderの出力はユーザ定義の出力用プリミティブ型用の頂点情報群となります。Vertex Shaderでは1入力1出力となっていましたが、Geometry Shaderは複数の情報を出力する事になり、出力情報によって生成されるプリミティブは1つ以上でも問題ありません。



例えば出力プリミティブ型をtriangleと定義した上で新しく計算によって求めた頂点を計9つ出力した場合は、3つの三角形がGeometry Shaderによって生成された事になります。この処理は前述の通りプリミティブ単位にて行われるため、元々1つだった三角形が3つに増えたとも考えられます。



また、Geometry ShaderにはMaxVertexCountという、一回の処理で最大何点の頂点を出力するかを事前に設定しておく必要があります。
例えばMaxVertexCountを9と設定した場合は、Geometry Shaderは0点 ~ 9点までの頂点数を出力することが出来るようになります。
この数値は後述する『Geometry Shaderの制限』によって、一般的には1024が一応の最大値となります。



なお、頂点情報を出力する上で気を付けなければならない点として、元々のメッシュの形状を維持した状態で新しく頂点を追加する場合は、Vertex Shaderから送られてきた頂点情報についてもGeometry Shaderにて出力する必要があります。
Geometry ShaderはVertex Shaderの出力に追加していくという挙動ではなく、Geometry Shaderの出力がラスタライズ処理が行われ、Fragment Shaderに渡されます。
逆説的に言えば、Geometry Shaderの出力を0にすることによって、動的に頂点数を減らすことも出来ます。


=== Geometry Shaderの制限


Geometry Shaderには1回の出力に関して、最大出力頂点数と最大出力要素数という制限があります。
最大出力頂点数は文字通り頂点数の限界値であり、GPUに依存した数値ではありますが1024などが一般的なので、1つの三角形から最大で1024点までしか頂点を増やすことが出来ます。
最大出力要素数における要素とは座標や色などの頂点が持っている情報の事であり、一般的には(x, y, z, w)の位置要素と(r, g, b, a)の色要素の計8要素となります。この要素の最大出力数もGPUに依存しますが同じく1024が一般的なので、出力は最大でも128(1024/8)に制限される事になります。



この二つの制限は両方を満たす必要があるため、頂点数的には1024点の出力が可能でも、要素数側の制約によって、実際のGeometry Shaderの出力は128点までは限界となります。
ですので、例えばプリミティブ数が2のメッシュ(Quadメッシュなど)に対してGeometry Shaderを利用した場合は、最大でも256点(128点 * 2プリミティブ)の頂点数までしか頂点を扱うことは出来ません。



この128点という数字が、前項のMaxVertexCountに設定できる数値の限界値となります。


== 簡単なGeometry Shader


以下にシンプルな挙動のGeometry Shaderのプログラムが記載してあります。
前項までの説明について実際のプログラムと照らし合わせながら改めて説明していきます。



なお、Geometry Shader以外について、Unityでシェーダーを記述する際に必要なShaderLabのシンタックスなどに関する説明は本章では省略しますので、もし分からない部分がありましたら下記の公式ドキュメントを参照してみてください。



@<href>{https://docs.unity3d.com/ja/current/Manual/SL-Reference.html,https://docs.unity3d.com/ja/current/Manual/SL-Reference.html}


//emlist[][cs]{
Shader "Custom/SimpleGeometryShader"
{
    Properties
    {
        _Height("Height", float) = 5.0
        _TopColor("Top Color", Color) = (0.0, 0.0, 1.0, 1.0)
        _BottomColor("Bottom Color", Color) = (1.0, 0.0, 0.0, 1.0)
    }
    SubShader
    {
        Tags { "RenderType" = "Opaque"}
        LOD 100

        Cull Off
        Lighting Off

        Pass
        {
            CGPROGRAM
            #pragma target 5.0
            #pragma vertex vert
            #pragma geometry geom
            #pragma fragment frag
            #include "UnityCG.cginc"

            uniform float _Height;
            uniform float4 _TopColor, _BottomColor;

            struct v2g
            {
                float4 pos : SV_POSITION;
            };

            struct g2f
            {
                float4 pos : SV_POSITION;
                float4 col : COLOR;
            };

            v2g vert(appdata_full v)
            {
                v2g o;
                o.pos = v.vertex;

                return o;
            }

            [maxvertexcount(12)]
            void geom(triangle v2g input[3],
                      inout TriangleStream<g2f> outStream)
            {
                float4 p0 = input[0].pos;
                float4 p1 = input[1].pos;
                float4 p2 = input[2].pos;

                float4 c = float4(0.0f, 0.0f, -_Height, 1.0f)
                            + (p0 + p1 + p2) * 0.33333f;

                g2f out0;
                out0.pos = UnityObjectToClipPos(p0);
                out0.col = _BottomColor;

                g2f out1;
                out1.pos = UnityObjectToClipPos(p1);
                out1.col = _BottomColor;

                g2f out2;
                out2.pos = UnityObjectToClipPos(p2);
                out2.col = _BottomColor;

                g2f o;
                o.pos = UnityObjectToClipPos(c);
                o.col = _TopColor;

                // bottom
                outStream.Append(out0);
                outStream.Append(out1);
                outStream.Append(out2);
                outStream.RestartStrip();

                // sides
                outStream.Append(out0);
                outStream.Append(out1);
                outStream.Append(o);
                outStream.RestartStrip();

                outStream.Append(out1);
                outStream.Append(out2);
                outStream.Append(o);
                outStream.RestartStrip();

                outStream.Append(out2);
                outStream.Append(out0);
                outStream.Append(o);
                outStream.RestartStrip();
            }

            float4 frag(g2f i) : COLOR
            {
                return i.col;
            }
            ENDCG
        }
    }
}
//}


このシェーダーでは、渡された三角形の中心座標を計算してさらに上方向に移動させ、渡されてきた三角形の各頂点と計算して求めた新しい座標を接続させています。
つまり、平面的な三角形から簡単な三角錐を生成していることになります。



なので、このシェーダーをQuadメッシュ(2つの三角形から構成されている)に適用すると、図6.2から図6.3のようになります。
//image[aoyama/img1][この様な平たい板から][scale=0.4]
//image[aoyama/img2][立体的な二つの三角錐が表示されるようになります][scale=0.4]



このシェーダーの中で、特にGeometry Shaderに関する部分だけを抜き出して説明していきます。


//emlist[][cs]{
#pragma target 5.0
#pragma vertex vert

// Geometry Shaderの利用を宣言
#pragma geometry geom

#pragma fragment frag
#include "UnityCG.cginc"
//}


上記の宣言部分にて、@<tt>{geom}という名前の関数がGeometry Shader用関数であることを宣言しています。
これによってGeometry Shaderステージになった時に@<tt>{geom}関数が呼び出されるようになります。


//emlist[][cs]{
[maxvertexcount(12)]
void geom(triangle v2g input[3], inout TriangleStream<g2f> outStream)
//}


これがGeometry Shader用の関数宣言です。


=== 入力

//emlist[][cs]{
triangle v2f input[3]
//}


ここが入力に関する部分です。



今回は三角形を元に三角錐を生成したいので、入力は@<tt>{triangle}としています。
これにより、単位プリミティブである三角形の各頂点情報が入力されるうになり、三角形は3点の頂点から構成されますので、受け取っている仮引数は長さ3の配列となります。
なので、もし入力を@<tt>{triangle}ではなく@<tt>{point}にした場合は構成する頂点は1点のみなので、@<tt>{geom(point v2f input[1])}の様に長さ1の配列で受け取ることになります。


=== 出力

//emlist[][cs]{
inout TriangleStream<g2f> outStream
//}


ここが出力に関する部分です。



今回生成するメッシュのプリミティブは三角形としたいため、@<tt>{TriangleStream}型で宣言しています。
@<tt>{TriangleStrema}型は出力が三角形ストリップである事を意味しているため、出力した各頂点情報を元に三角形を生成してくれるようになります。
他にも@<tt>{PointStream}型や@<tt>{LineStream}型などがありますので、目的に応じて出力のプリミティブ型を選択する必要があります。



また、@<tt>{[maxvertexcount(12)]}の部分にて最大出力数を12に設定してあります。
これは三角錐を構成する三角形の数は底辺の1つと側面の3つで計4つであり、一つの三角形に付き頂点数が3点必要なので、3 * 4で12点の頂点を出力することになるため12と設定してあります。


=== 処理

//emlist[][cs]{
g2f out0;
out0.pos = UnityObjectToClipPos(p0);
out0.col = _BottomColor;

g2f out1;
out1.pos = UnityObjectToClipPos(p1);
out1.col = _BottomColor;

g2f out2;
out2.pos = UnityObjectToClipPos(p2);
out2.col = _BottomColor;

g2f o;
o.pos = UnityObjectToClipPos(c);
o.col = _TopColor;

// bottom
outStream.Append(out0);
outStream.Append(out1);
outStream.Append(out2);
outStream.RestartStrip();

// sides
outStream.Append(out0);
outStream.Append(out1);
outStream.Append(o);
outStream.RestartStrip();

outStream.Append(out1);
outStream.Append(out2);
outStream.Append(o);
outStream.RestartStrip();

outStream.Append(out2);
outStream.Append(out0);
outStream.Append(o);
outStream.RestartStrip();
//}


ここが実際の頂点を出力している処理の部分です。



まず最初に出力用のg2f型の変数を宣言し、頂点座標と色情報を格納しています。
この時Vertex Shaderと同じようにオブジェクト空間からカメラのクリップ空間への変換をしておく必要があります。



その後に、メッシュを構成する頂点の順序を意識しながら、頂点情報を出力しています。
@<tt>{outStream}変数の@<tt>{Append}関数に出力用変数を渡すことで現在のストリームに追加されていき、@<tt>{RestartStrip}関数を呼び出す事によって現在のプリミティブストリップを終了し、新しいストリームを開始しています。



これは、@<tt>{TriangleStream}は三角形ストリップなので、@<tt>{Append}関数で頂点を追加していくほどそのストリームに追加されている全ての頂点を元に、接続された複数の三角形を生成していくことになります。
なので、今回の様に三角形同士が@<tt>{Append}された順序を元に接続されると困る時は、一旦@<tt>{RestartStrip}を呼び出して新しいストリームを開始する必要があります。
もちろん@<tt>{Append}順を工夫することで@<tt>{RestartStrip}関数の呼び出しを減らすことは可能です。


== Grass Shader


本項では、前項の『簡単なGeometry Shader』から少し発展させて、Geometry Shaderを使ってリアルタイムに草を生成するGrass Shaderについて説明します。

以下は説明するGrass Shaderのプログラムです。


//emlist[][cs]{
Shader "Custom/Grass" {
    Properties
    {
        // 草の高さ
        _Height("Height", float) = 80
        // 草の幅
        _Width("Width", float) = 2.5

        // 草の下部の高さ
        _BottomHeight("Bottom Height", float) = 0.3
        // 草の中間部の高さ
        _MiddleHeight("Middle Height", float) = 0.4
        // 草の上部の高さ
        _TopHeight("Top Height", float) = 0.5

        // 草の下部の幅
        _BottomWidth("Bottom Width", float) = 0.5
        // 草の中間部の幅
        _MiddleWidth("Middle Width", float) = 0.4
        // 草の上部の幅
        _TopWidth("Top Width", float) = 0.2

        // 草の下部の曲がり具合
        _BottomBend("Bottom Bend", float) = 1.0
        // 草の中間部の曲がり具合
        _MiddleBend("Middle Bend", float) = 1.0
        // 草の上部の曲がり具合
        _TopBend("Top Bend", float) = 2.0

        // 風の強さ
        _WindPower("Wind Power", float) = 1.0

        // 草の上部の色
        _TopColor("Top Color", Color) = (1.0, 1.0, 1.0, 1.0)
        // 草の下部の色
        _BottomColor("Bottom Color", Color) = (0.0, 0.0, 0.0, 1.0)

        // 草の高さにランダム性を与えるノイズテクスチャ
        _HeightMap("Height Map", 2D) = "white"
        // 草の向きにランダム性を与えるノイズテクスチャ
        _RotationMap("Rotation Map", 2D) = "black"
        // 風の強さにランダム性を与えるノイズテクスチャ
        _WindMap("Wind Map", 2D) = "black"
    }
    SubShader
    {
        Tags{ "RenderType" = "Opaque" }

        LOD 100
        Cull Off

        Pass
        {
            CGPROGRAM
            #pragma target 5.0
            #include "UnityCG.cginc"

            #pragma vertex vert
            #pragma geometry geom
            #pragma fragment frag

            float _Height, _Width;
            float _BottomHeight, _MiddleHeight, _TopHeight;
            float _BottomWidth, _MiddleWidth, _TopWidth;
            float _BottomBend, _MiddleBend, _TopBend;

            float _WindPower;
            float4 _TopColor, _BottomColor;
            sampler2D _HeightMap, _RotationMap, _WindMap;

            struct v2g
            {
                float4 pos : SV_POSITION;
                float3 nor : NORMAL;
                float4 hei : TEXCOORD0;
                float4 rot : TEXCOORD1;
                float4 wind : TEXCOORD2;
            };

            struct g2f
            {
                float4 pos : SV_POSITION;
                float4 color : COLOR;
            };

            v2g vert(appdata_full v)
            {
                v2g o;
                float4 uv = float4(v.texcoord.xy, 0.0f, 0.0f);

                o.pos = v.vertex;
                o.nor = v.normal;
                o.hei = tex2Dlod(_HeightMap, uv);
                o.rot = tex2Dlod(_RotationMap, uv);
                o.wind = tex2Dlod(_WindMap, uv);

                return o;
            }

            [maxvertexcount(7)]
            void geom(triangle v2g i[3], inout TriangleStream<g2f> stream)
            {
                float4 p0 = i[0].pos;
                float4 p1 = i[1].pos;
                float4 p2 = i[2].pos;

                float3 n0 = i[0].nor;
                float3 n1 = i[1].nor;
                float3 n2 = i[2].nor;

                float height = (i[0].hei.r + i[1].hei.r + i[2].hei.r) / 3.0f;
                float rot = (i[0].rot.r + i[1].rot.r + i[2].rot.r) / 3.0f;
                float wind = (i[0].wind.r + i[1].wind.r + i[2].wind.r) / 3.0f;

                float4 center = ((p0 + p1 + p2) / 3.0f);
                float4 normal = float4(((n0 + n1 + n2) / 3.0f).xyz, 1.0f);

                float bottomHeight = height * _Height * _BottomHeight;
                float middleHeight = height * _Height * _MiddleHeight;
                float topHeight = height * _Height * _TopHeight;

                float bottomWidth = _Width * _BottomWidth;
                float middleWidth = _Width * _MiddleWidth;
                float topWidth = _Width * _TopWidth;

                rot = rot - 0.5f;
                float4 dir = float4(normalize((p2 - p0) * rot).xyz, 1.0f);

                g2f o[7];

                // Bottom.
                o[0].pos = center - dir * bottomWidth;
                o[0].color = _BottomColor;

                o[1].pos = center + dir * bottomWidth;
                o[1].color = _BottomColor;

                // Bottom to Middle.
                o[2].pos = center - dir * middleWidth + normal * bottomHeight;
                o[2].color = lerp(_BottomColor, _TopColor, 0.33333f);

                o[3].pos = center + dir * middleWidth + normal * bottomHeight;
                o[3].color = lerp(_BottomColor, _TopColor, 0.33333f);

                // Middle to Top.
                o[4].pos = o[3].pos - dir * topWidth + normal * middleHeight;
                o[4].color = lerp(_BottomColor, _TopColor, 0.66666f);

                o[5].pos = o[3].pos + dir * topWidth + normal * middleHeight;
                o[5].color = lerp(_BottomColor, _TopColor, 0.66666f);

                // Top.
                o[6].pos = o[5].pos + dir * topWidth + normal * topHeight;
                o[6].color = _TopColor;

                // Bend.
                dir = float4(1.0f, 0.0f, 0.0f, 1.0f);

                o[2].pos += dir
                            * (_WindPower * wind * _BottomBend)
                            * sin(_Time);
                o[3].pos += dir
                            * (_WindPower * wind * _BottomBend)
                            * sin(_Time);
                o[4].pos += dir
                            * (_WindPower * wind * _MiddleBend)
                            * sin(_Time);
                o[5].pos += dir
                            * (_WindPower * wind * _MiddleBend)
                            * sin(_Time);
                o[6].pos += dir
                            * (_WindPower * wind * _TopBend)
                            * sin(_Time);

                [unroll]
                for (int i = 0; i < 7; i++) {
                    o[i].pos = UnityObjectToClipPos(o[i].pos);
                    stream.Append(o[i]);
                }
            }

            float4 frag(g2f i) : COLOR
            {
                return i.color;
            }
            ENDCG
        }
    }
}
//}


このシェーダーを縦横に複数並べたPlaneメッシュに適用すると、図6.4のようになります。
//image[aoyama/img3][Grass Shaderの結果][scale=0.6]



この中から草を生成する処理についての説明をします。


=== 基本方針


今回は一つのプリミティブにつき1本の草を生成することにします。
草の形状の生成については図6.5のように下部・中間部・上部に分けて頂点を合計7点生成し、上に行くほど斜めにしていくことで、草の斜め具合を簡易的に表現します。



//image[aoyama/img4][草の形の作り方][scale=0.4]


=== パラメーター

詳細はコメントにて記載してありますが、一本の草の中の各部分(下部・中間部・上部)の横幅と高さをコントロールする係数、草全体の横幅と高さをコントロールする係数を主なパラメーターとして用意しています。
また一本一本の草が同じ形になるのは見栄えが悪いので、ランダム性を持たせるためのノイズテクスチャを使います。


=== 処理

//emlist[][cs]{
float height = (i[0].hei.r + i[1].hei.r + i[2].hei.r) / 3.0f;
float rot = (i[0].rot.r + i[1].rot.r + i[2].rot.r) / 3.0f;
float wind = (i[0].wind.r + i[1].wind.r + i[2].wind.r) / 3.0f;

float4 center = ((p0 + p1 + p2) / 3.0f);
float4 normal = float4(((n0 + n1 + n2) / 3.0f).xyz, 1.0f);
//}


この部分では草の高さと向き、風の強弱の基準となる数値を計算しています。
Geometry Shader内で計算しても良いのですが、頂点に対してメタ情報的に持たせた方がGeometry Shader上で計算を行なう上での初期値の様な扱いが出来るのでVertex Shaderで計算しています。


//emlist[][cs]{
float4 center = ((p0 + p1 + p2) / 3.0f);
float4 normal = float4(((n0 + n1 + n2) / 3.0f).xyz, 1.0f);
//}


ここでは草の中心部分と、草を生やしていく方向を計算しています。
ここの部分をノイズテクスチャなどで決定するようにすると、草が生える方向にランダム性を持たせることが出来ます。


//emlist[][cs]{
float bottomHeight = height * _Height * _BottomHeight;

...

o[6].pos += dir * (_WindPower * wind * _TopBend) * sin(_Time);
//}


長いのでプログラムは略記してあります。
この部分では下部・中間部・上部についての高さと幅をそれぞれ計算し、それを元に座標を求めています。


//emlist[][cs]{
[unroll]
for (int i = 0; i < 7; i++) {
    o[i].pos = UnityObjectToClipPos(o[i].pos);
    stream.Append(o[i]);
}
//}


この部分にて計算した7点の頂点を@<tt>{Append}しています。
今回は三角形が繋がりながら生成されていっても問題ないため、@<tt>{RestartStrip}はしていません。



なお、@<tt>{for}ステートメントに対して@<tt>{[unroll]}というアトリビュートを適用しています。
これはコンパイル時に、ループの回数分ループ内の処理を展開するというアトリビュートで、メモリサイズが大きくなるというデメリットはあるのですが、高速に動作するという利点があります。


== まとめ


ここまでGeometry Shaderについての説明から、基本と応用のプログラムまでを説明してきました。
CPU上で動くプログラムを書くのとは多少なりとも特徴が異なる所がありますが、基本的な部分を抑えさせすれば活用できるはずです。



実は通説としてGeometry Shaderは遅いと言われているそうです。
筆者自身はあまり感じたことはないのですが、利用範囲が大規模になると大変なのかもしれません。
もしGeometry Shaderを大規模に使うということになりそうでしたら、ぜひ一度ベンチマークなどを取ってみてください。



それでもGPU上で動的に且つ自由に新しいメッシュを作ったり、削除したり出来るというのはアイデアの幅をかなり広げることになると思います。
個人的に最も重要なことは、どの技術を使ったのかではなく、それによって何を作り、表現するのかだと思っています。
ぜひ本章にてGeometry Shaderという一つの道具を知り学んだ上で、なにか新しい可能性を感じてもらえたら幸いです。


== 参考
 * チュートリアル13 : ジオメトリシェーダー - @<href>{https://msdn.microsoft.com/ja-jp/library/bb172497,https://msdn.microsoft.com/ja-jp/library/bb172497}
 * ジオメトリシェーダーオブジェクト in MSDN - @<href>{https://msdn.microsoft.com/ja-jp/library/ee418313,https://msdn.microsoft.com/ja-jp/library/ee418313}
 * ジオメトリシェーダのジオメトリ切断による透明ジオメトリのためのレンダリング手法 - @<href>{http://t-pot.com/program/147_CGGONG2008/index.html,http://t-pot.com/program/147_CGGONG2008/index.html}

