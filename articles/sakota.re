
= 格子法による流体シミュレーション

== この章について

本章では、ComputeShaderを使った格子法による流体シミュレーションについて解説します。

== サンプルデータ

=== コード
@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming/,https://github.com/IndieVisualLab/UnityGraphicsProgramming/}  

のAssets/StabeFluidsに格納されています。

=== 実行環境
 * ComputeShaderが実行できる、シェーダーモデル5.0対応環境
 * 執筆時環境、Unity5.6.2, Unity2017.1.1で動作確認済み

== はじめに

本章では、格子法による流体シミュレーションと、それらを実現するにあたって必要となる、数式の計算方法や捉え方を解説していきます。まず格子法とは何でしょう。その意味を探る為に、一度流体力学での「流れ」の解析方法に少し迫ってみましょう。

=== 流体力学での捉え方

流体力学とは、自然現象である「流れ」を数式化して、計算可能なものとする事に特徴をおいています。この「流れ」、一体どうすれば数値化し解析することが出来るでしょうか。@<br>{}
端的に行ってしまいますと、「時間が一瞬進んだ時の流速」を導く事で数値化する事ができます。少し数学的に言うと、時間で微分した際の流速ベクトルの変化量の解析と言い換える事ができます。@<br>{}
ただ、この流れを解析する方法として、二つの手法が考えられます。@<br>{}
一つは、お風呂のお湯をイメージした際に、お風呂にはったお湯を格子状に分割し、その固定された各格子空間の流速ベクトルを測定する方法。@<br>{}
そしてもう一つは、お風呂にアヒルを浮かべ、アヒルの動き自体を解析する方法です。この二つの方法の内、前者を「オイラーの方法」、後者を「ラグランジュの方法」と呼びます。

=== 様々な流体シミュレーション

さて、一旦コンピューターグラフィックスの方に話を戻しましょう。流体シミュレーションにも、「オイラーの方法」や「ラグランジュの方法」の様にいくつかのシミュレーション方法が存在しますが、大きく分けて、以下の３種類に大別する事ができます。

 * 格子法 (e.g. Stable Fluid)
 * 粒子法 (e.g. SPH)
 * 格子法＋粒子法 (e.g. FLIP)

漢字の意味合いから少し想像することができるかもしれませんが、格子法は「オイラーの方法」の様に、流れをシミュレーションする際に格子状の「場」を作り、時間で微分した際にその各格子がどういった速度になっているかをシミュレーションする手法をいいます。
また粒子法は「ラグランジュの方法」の様に、その粒子の方に着目し、粒子自体の移流をシミュレーションする方法を言います。@<br>{}
格子法・粒子法と共に、お互いに得意不得意な範囲があります。@<br>{}
格子法は流体のシミュレーションにおいて、圧力・粘性・拡散等の計算は得意ですが、移流の計算が不得意です。@<br>{}
これとは逆に、粒子法は移流の計算が得意です。（これらの得意不得意は、オイラーの方法とラグランジュの方法の解析の仕方を思い浮かべると想像がつくかもしれません。）@<br>{}
これらを補う為に、FLIP法に代表される、格子法＋粒子法と言った得意分野を補い合う手法も生まれています。  

本稿ではSIGGRAPH 1999.で発表されたJon Stam氏の格子法における非圧縮性粘性流体シミュレーションの論文であるStable Fluidsを元に、流体シミュレーションの実装方法やシミュレーションにおける必要な数式の説明を行なっていきます。

== ナビエ・ストークス方程式について

まずは、格子法におけるナビエ・ストークスの方程式について見ていきましょう。  

//texequation{
\dfrac {\partial \overrightarrow {u}} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \overrightarrow {u} + \nu \nabla ^{2} \overrightarrow {u} + \overrightarrow{f}
//}

//texequation{
\dfrac {\partial \rho} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \rho + \kappa \nabla ^{2} \rho + S
//}

//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}

上記の内、一つ目の方程式は速度場、二つ目は密度場を表します。また、3つ目は「連続の式（質量保存則）」となります。
これらの3つの式を一つずつ紐解いて見ましょう。  


== 連続の式（質量保存則）


まずは式としても短く、「非圧縮性」流体をシミュレーションする際の条件として働く「連続の式（質量保存則）」から紐解いて見ましょう。@<br>{}
流体をシミュレーションする際に、その対象が圧縮性か非圧縮性かを明確に区別する必要があります。例えば、気体等の密度が圧力によって変化する物が対象である場合は圧縮性流体となります。逆に、水などの密度がどの場所でも一定である物は、非圧縮性流体となります。@<br>{}
本章では非圧縮性流体のシミュレーションを取り扱いますので、速度場の各セルの発散は0に保つ必要があります。つまり、速度場の流入と流出を相殺させ、0になるように維持します。流入があれば流出させる為、流速は伝搬して行く事となります。この条件は連続の式（質量保存則）として、以下の方程式で表す事ができます。



//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}



上記は「発散（ダイバージェンス）が0」であるという意味になります。まずは「発散（ダイバージェンス）」の数式を確認しておきましょう。


=== 発散（Divergence）


//texequation{
\nabla \cdot \overrightarrow{u} = \nabla \cdot (u, v) = \dfrac{\partial u}{\partial x} + \dfrac{\partial v}{\partial y}
//}



@<m>{\nabla}（ナブラ演算子）はベクトル微分演算子といいます。例えばベクトル場が2次元と想定した場合に、図のように@<m>{ \left( \dfrac {\partial \} {\partial x\}_, \dfrac {\partial \} {\partial y\} \right) }の偏微分を取る際の、偏微分の表記を簡略化した演算子として作用します。@<m>{\nabla}演算子は演算子ですので、それだけでは意味を持ちませんが、一緒に組み合わせる式が内積なのか、外積なのか、それとも単に@<m>{\nabla f}といった関数なのかで演算内容が変わってきます。@<br>{}
今回は偏微分の内積をとる「発散（ダイバージェンス）」について説明しておきましょう。まず、なぜこの式が「発散」という意味になるのかを見てみます。



発散を理解する為に、まずは下記のような格子空間の一つのセルを切り出して考えてみましょう。



//image[divergence-s][ベクトル場から微分区間（Δx,Δy）のセルを抽出][scale=0.7]{
//}




発散とは、ベクトル場の一つのセルにどれくらいのベクトルが流出、流入しているかを算出する事を言います。なお流出を＋、流入を−とします。
  
発散は上記のように、ベクトル場のセルを切り取った際の偏微分をみた際に、x方向の特定のポイントxと微量に進んだ@<m>{\Delta x}との変化量、また、y方向の特定のポイントyと微量に進んだ@<m>{\Delta y}との変化量の内積で求める事ができます。
なぜ偏微分との内積で流出が求まるかは、上記の図を微分演算する事で証明できます。



//texequation{
\frac{i(x + \Delta x, y)\Delta y - i(x,y)\Delta y + j(x, y + \Delta y)\Delta x - j(x,y)\Delta x }{\Delta x \Delta y}
//}

//texequation{
 = \frac{i(x+\Delta x, y) - i(x,y)}{\Delta x} + \frac{j(x, y+\Delta y) - j(x,y)}{\Delta y}
//}



上記の式から極限をとり、



//texequation{
\lim_{\Delta x \to 0} \frac{i(x+\Delta x, y) - i(x,y)}{\Delta x} + \lim_{\Delta y \to 0} \frac{j(x,y+\Delta y) - j(x,y)}{\Delta y} = \dfrac {\partial i} {\partial x} + \dfrac {\partial j} {\partial y}
//}



とする事で、最終的に偏微分との内積の式と等式になる事がわかります。


== 速度場


次に、格子法の本丸である速度場について説明していきます。
その前に、速度場のナビエ・ストークス方程式を実装していくにあたって、先ほど確認した発散（divergence）に加え、勾配（gradient）とラプラシアン（Laplacian）について確認しておきましょう。


=== 勾配（Gradient）


//texequation{
\nabla f(x, y) = \left( \dfrac{\partial f}{\partial x}_,\dfrac{\partial f}{\partial y}\right)
//}


@<m>{\nabla f (grad \ f)}は勾配を求める式となります。意味としては、各偏微分方向に微小に進んだ座標を、関数@<m>{f}にてサンプリングし、求められた各偏微分方向の値を合成する事によって、最終的にどのベクトルを向くのかを意味しています。つまり、偏微分した際の値の大きい方向に向いたベクトルを算出する事ができます。


=== ラプラシアン（Laplacian）


//texequation{
\Delta f = \nabla^2 f = \nabla \cdot \nabla f = \frac{\partial^2 f}{\partial x^2} + \frac{\partial^2 f}{\partial y^2}
//}



ラプラシアンはナブラを上下反転させた記号で表されます。(デルタと同じですが、文脈から読み取り、間違えないようにしましょう。)@<br>{}
@<m>{\nabla^2 f}、もしくは@<m>{\nabla \cdot \nabla f}とも書き、二階偏微分として演算されます。@<br>{}
また、解体して考えると、関数の勾配をとって、発散を求めた形とも取れるでしょう。@<br>{}
意味合い的に考えると、ベクトル場の中で勾配方向に集中した箇所は流入が多い為、発散をとった場合−に、逆に勾配の低い箇所は湧き出しが多いので発散を取った時に＋になる事が想像できます。@<br>{}
ラプラシアン演算子にはスカラーラプラシアンとベクトルラプラシアンがあり、ベクトル場に作用させる場合は、勾配・発散・回転（∇とベクトルの外積）を用いた、@<br>{}
//texequation{
\nabla^2 \overrightarrow{u} = \nabla \nabla \cdot \overrightarrow{u} - \nabla \times \nabla \times \overrightarrow{u}
//}
といった式で導くのですが、直交座標系の場合のみ、ベクトルの成分毎に勾配と発散を求め、合成する事で求める事ができます。



//texequation{
\nabla^2 \overrightarrow{u} = \left(
\dfrac{\partial ^2 u_x}{\partial x^2}+\dfrac{\partial ^2 u_x}{\partial y^2}+\dfrac{\partial ^2 u_x}{\partial z^2}_,
\dfrac{\partial ^2 u_y}{\partial x^2}+\dfrac{\partial ^2 u_y}{\partial y^2}+\dfrac{\partial ^2 u_y}{\partial z^2}_,
\dfrac{\partial ^2 u_z}{\partial x^2}+\dfrac{\partial ^2 u_z}{\partial y^2}+\dfrac{\partial ^2 u_z}{\partial z^2}
\right)
//}



以上で、格子法でのナビエ・ストークス方程式を解くための必要な数式の確認は完了しました。
ここから、速度場の方程式を各項ごとに見ていきましょう。


=== ナビエ・ストークス方程式から速度場の確認


//texequation{
\dfrac {\partial \overrightarrow {u}} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \overrightarrow {u} + \nu \nabla ^{2} \overrightarrow {u} + \overrightarrow {f}
//}



上記の内、@<m>{\overrightarrow {u\}}は流速、@<m>{\nu}は動粘性係数（kinematic viscosity）、@<m>{\overrightarrow{f\}}は外力（force）になります。@<br>{}
左辺側は時間で偏微分をとった際の流速である事がわかります。右辺側は第一項を移流項、第二項を拡散粘性項、第三項を圧力項、第四項を外力項とします。  



これらは、計算時には一括でできるものであっても、実装時にはステップに分けて実装して行く必要があります。@<br>{}
まず、ステップとして、外力を受けなければ、初期条件のまま変化を起こす事ができませんので、第四項の外力項から考えて見たいと思います。


=== 速度場外力項


これはシンプルに外部からのベクトルを加算する部分となります。つまり初期条件で速度場がベクトル量0の状態に対し、ベクトルの起点としてUIであったりなんらかのイベントから、RWTexture2Dの該当IDにベクトルを加算する部分となります。@<br>{}
コンピュートシェーダーの外力項のカーネルは、以下の様に実装しておきます。また、コンピュートシェーダーにて使用予定の各係数やバッファの定義も記述しておきます。  


//emlist{
float visc;                   //動粘性係数
float dt;                     //デルタタイム
float velocityCoef;           //速度場外力係数
float densityCoef;            //密度場外圧係数

//xy = velocity, z = density, 描画シェーダに渡す流体ソルバー
RWTexture2D<float4> solver;
//density field, 密度場
RWTexture2D<float>  density;  
//velocity field, 速度場
RWTexture2D<float2> velocity; 
//xy = pre vel, z = pre dens. when project, x = p, y = div
//1ステップ前のバッファ保存、及び質量保存時の一時バッファ
RWTexture2D<float3> prev;
//xy = velocity source, z = density source 外力入力バッファ
Texture2D source;             

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void AddSourceVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        velocity[id] += source[id].xy * velocityCoef * dt;
        prev[id] = float3(source[id].xy * velocityCoef * dt, prev[id].z);
    }
}
//}


次のステップとして、第二項の拡散粘性項を実装します。


=== 速度場拡散粘性項


//texequation{
\nu \nabla ^{2} \overrightarrow {u}
//}



@<m>{\nabla}演算子や@<m>{\Delta}演算子の左右に値がある時には、「右の要素にのみ作用する」というルールがありますので、この場合、動粘性係数は一旦置いておいて、ベクトルラプラシアンの部分を先に考えます。@<br>{}
流速@<m>{\overrightarrow{u\}}に対してベクトルラプラシアンで、ベクトルの各成分毎の勾配と発散をとり合成させ、流速を隣接へ拡散させています。そこに動粘性係数を乗算する事によって、拡散の勢いを調整します。@<br>{}
ここでは流速の各成分の勾配を取った上に拡散させていますので、隣接からの流入も隣接への流出も起こり、ステップ1で受けたベクトルが隣接へと影響していくという現象が分かるかと思います。@<br>{}
実装面においては、少し工夫が必要となります。数式通りに実装すると、粘性係数と微分時間・格子数を乗算させた拡散率が高くなってしまった場合に、振動が起こり、収束が取れず最後にはシミュレーション自体が発散してしまいます。@<br>{}
拡散をStableな状態にする為に、ここではガウス・ザイデル法やヤコビ法、SOR法等の反復法が用いられます。ここではガウス・ザイデル法でシミュレーションしてみましょう。@<br>{}
ガウス・ザイデル法とは、式を自セルに対する未知数からなる線形方程式に変換し、算出された値をすぐに次の反復時に使い、連鎖させることで近似の答えに収束させていく方法です。反復回数は多ければ多いほど正確な値へと収束していきますが、リアルタイムレンダリングにおけるグラフィックスで必要なのは、正確な結果ではなく、より良いフレームレートと見た目の美しさですので、イテレーション回数はマシンパフォーマンスや見た目を考慮し、調整しましょう。


//emlist{
#define GS_ITERATE 4

[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void DiffuseVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float a = dt * visc * w * h;

        [unroll]
        for (int k = 0; k < GS_ITERATE; k++) {
            velocity[id] = (prev[id].xy + a * (
                            velocity[int2(id.x - 1, id.y)] +
                            velocity[int2(id.x + 1, id.y)] +
                            velocity[int2(id.x, id.y - 1)] +
                            velocity[int2(id.x, id.y + 1)]
                            )) / (1 + 4 * a);
            SetBoundaryVelocity(id, w, h);
        }
    }
}
//}


上記のSetBoundaryVelocity関数は境界用のメソッドになります。詳しくはリポジトリをご参照下さい。


=== 質量保存


//texequation{
\nabla \cdot \overrightarrow{u} = 0
//}



ここで一旦、項を進める前に質量保存側に立ち返りましょう。これまでの工程で、外力項で受けた力を速度場に拡散させましたが、現状、各セルの質量は保存されておらず、湧き出しっぱなしの場所と流入が多い場所とで、質量が保存されていない状態になっています。@<br>{}
上記の方程式の様に、質量は必ず保存させ各セルの発散を0に持っていかないといけませんから、ここで一旦質量を保存をしておきましょう。@<br>{}
なお、質量保存ステップをComputeShaderで行う際、隣接スレッドとの偏微分演算を行う為、場を確定しておかなければなりません。
グループシェアードメモリ内で偏微分演算ができれば高速化が見込めたのですが、別のグループスレッドから偏微分を取った時に、やはり値が取得できず汚い結果となってしまった為、ここはバッファを確定しながら、3ステップに分け進めます。@<br>{}
速度場から発散算出 > Poisson方程式をガウス・ザイデル法で算出 > 速度場に減算させ質量保存@<br>{}
の3ステップにカーネルをわけ、場を確定しながら質量保存に持っていきます。なお、SetBound~系は境界に対するメソッドの呼び出しになります。


//emlist{
//質量保存Step1.
//step1では、速度場から発散の算出
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep1(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float2 uvd;
        uvd = float2(1.0 / w, 1.0 / h);

        prev[id] = float3(0.0,
                    -0.5 * 
                    (uvd.x * (velocity[int2(id.x + 1, id.y)].x -
                              velocity[int2(id.x - 1, id.y)].x)) +
                    (uvd.y * (velocity[int2(id.x, id.y + 1)].y - 
                              velocity[int2(id.x, id.y - 1)].y)),
                    prev[id].z);

        SetBoundaryDivergence(id, w, h);
        SetBoundaryDivPositive(id, w, h);
    }
}

//質量保存Step2.
//step2では、step1で求めた発散からPoisson方程式をガウス・ザイデル法で解く
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep2(uint2 id : SV_DispatchThreadID)
{
    uint w, h;

    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        for (int k = 0; k < GS_ITERATE; k++)
        {
            prev[id] = float3(
                        (prev[id].y + prev[uint2(id.x - 1, id.y)].x + 
                                      prev[uint2(id.x + 1, id.y)].x +
                                      prev[uint2(id.x, id.y - 1)].x +
                                      prev[uint2(id.x, id.y + 1)].x) / 4,
                        prev[id].yz);
            SetBoundaryDivPositive(id, w, h);
        }
    }
}

//質量保存Step3.
//step3で、∇･u = 0にする.
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void ProjectStep3(uint2 id : SV_DispatchThreadID)
{
    uint w, h;

    velocity.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        float  velX, velY;
        float2 uvd;
        uvd = float2(1.0 / w, 1.0 / h);

        velX = velocity[id].x;
        velY = velocity[id].y;

        velX -= 0.5 * (prev[uint2(id.x + 1, id.y)].x - 
                       prev[uint2(id.x - 1, id.y)].x) / uvd.x;
        velY -= 0.5 * (prev[uint2(id.x, id.y + 1)].x -
                       prev[uint2(id.x, id.y - 1)].x) / uvd.y;

        velocity[id] = float2(velX, velY);
        SetBoundaryVelocity(id, w, h);
    }
}
//}


これで速度場を質量保存がされた状態にできました。流出した箇所に流入がおき、流入が多い箇所からは流出がおきる為、流体らしい表現になりました。  


=== 移流項


//texequation{
-\left( \overrightarrow {u} \cdot \nabla \right) \overrightarrow {u}
//}



移流項はラグランジュの方法的な手法が用いられるのですが、1ステップ前の速度場のバックトレースを行い、該当セルから速度ベクトルを引いた箇所の値を、現在いる場所に移動するといった作業を各セルに対して行います。
バックトレースした際に、格子にぴったり収まる場所に遡れる訳ではありませんので、移流の際は近傍4セルとの線形補間を行い、正しい値を移流させます。


//emlist{
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void AdvectVelocity(uint2 id : SV_DispatchThreadID)
{
    uint w, h;
    density.GetDimensions(w, h);

    if (id.x < w && id.y < h)
    {
        int ddx0, ddx1, ddy0, ddy1;
        float x, y, s0, t0, s1, t1, dfdt;

        dfdt = dt * (w + h) * 0.5;

        //バックトレースポイント割り出し.
        x = (float)id.x - dfdt * prev[id].x;
        y = (float)id.y - dfdt * prev[id].y;
        //ポイントがシミュレーション範囲内に収まるようにクランプ.
        clamp(x, 0.5, w + 0.5);
        clamp(y, 0.5, h + 0.5);
        //バックトレースポイントの近傍セル割り出し.
        ddx0 = floor(x);
        ddx1 = ddx0 + 1;
        ddy0 = floor(y);
        ddy1 = ddy0 + 1;
        //近傍セルとの線形補間用の差分を取っておく.
        s1 = x - ddx0;
        s0 = 1.0 - s1;
        t1 = y - ddy0;
        t0 = 1.0 - t1;

        //バックトレースし、1step前の値を近傍との線形補間をとって、現在の速度場に代入。
        velocity[id] = s0 * (t0 * prev[int2(ddx0, ddy0)].xy +
                             t1 * prev[int2(ddx0, ddy1)].xy) +
                       s1 * (t0 * prev[int2(ddx1, ddy0)].xy +
                             t1 * prev[int2(ddx1, ddy1)].xy);
        SetBoundaryVelocity(id, w, h);
    }
}
//}

== 密度場


次に密度場の方程式をみてみましょう。



//texequation{
\dfrac {\partial \rho} {\partial t}=-\left( \overrightarrow {u} \cdot \nabla \right) \rho + \kappa \nabla ^{2} \rho + S
//}



上記の内、@<m>{\overrightarrow {u\}}は流速、@<m>{\kappa}は拡散係数、ρは密度、Sは外圧になります。@<br>{}
密度場は必ずしも必要ではありませんが、速度場を求めた際の各ベクトルに対し、密度場で拡散させた画面上のピクセルを乗せる事で、溶けながら流れる様な、より流体らしい表現が可能になります。@<br>{}
尚、密度場の数式を見て気づいた方もいらっしゃるかと思いますが、速度場と全く同じフローになっており、違いはベクトルがスカラーになっている点と、動粘性係数@<m>{\nu}が拡散係数@<m>{\kappa}になっている点、質量保存則を用いない点の3点のみしかありません。@<br>{}
密度場は密度の変化の場ですので、非圧縮性である必要はなく、質量保存の必要がありません。また、動粘性係数と拡散係数は、係数としての使い所は同じになります。@<br>{}
ですので、先ほど速度場で用いたカーネルの質量保存則以外のカーネルを、次元を落として作ることによって、密度場を実装する事が可能です。
紙面上密度場の解説はしませんが、リポジトリには密度場も実装しておりますので、そちらもご参照ください。


== シミュレーションの各項ステップ


上記の速度場及び密度場、質量保存則を用いることによって流体をシミュレーションする事ができるのですが、シミュレーションのステップについて、最後に見ておきましょう。

 * 外力イベントを発生させ、速度場と密度場の外力項にインプット
 * 速度場を以下のステップで更新
 ** 拡散粘性項
 ** 質量保存則
 ** 移流項
 ** 質量保存則
 * その後密度場を以下のステップで更新
 ** 拡散項
 ** 速度場の速度を用いで密度を移流

上記がStableFluidのシミュレーションステップになります。


== 結果

実行して、マウスでスクリーン上をドラッグすると、以下の様な流体シミュレーションを起こす事が可能です。

//image[fluid-s][実行例][scale=0.7]{
//}

== まとめ

流体シミュレーションは、プリレンダリングと違い、Unityの様なリアルタイムゲームエンジンにとっては負荷の高い分野です。
しかし、GPU演算能力の向上から、2次元であればある程度の解像度でも耐えうるFPSが出せる様になってきました。
また、途中で出てきたGPUにとって負荷の高い演算部分、ガウス・ザイデル反復法を別の処理で実装してみたり、速度場自体をカールノイズで代用してみたり等の工夫をすれば、より軽い演算での流体表現も可能になる事でしょう。

もしこの章をお読みいただいて、少しでも流体に興味を持たれた方は、ぜひ次章の「粒子法による流体シミュレーション」にもトライして見て下さい。  
格子法とはまた違った角度から流体に迫れますので、流体シミュレーションの奥深さや実装の面白さを体験できる事かと思います。

== 参考

 * Jos Stam. SIGGRAPH 1999. Stable Fluids
