
= SPH法による流体シミュレーション

前章では、格子法による流体シミュレーションの作成方法について解説しました。
本章では、もう一つの流体のシミュレーション方法である粒子法、特にSPH法を用いて流体の動きを表現していきます。
多少噛み砕いて説明を行っているので、不十分な表現などありますがご了承ください。

== 基礎知識
=== オイラー的視点とラグランジュ的視点
流体の動きの観測方法として、オイラー的視点とラグランジュ的視点というものが存在します。
オイラー的視点とは、流体に等間隔で観測点を@<b>{固定}し、その観測点での流体の動きを解析するものです。
一方、ラグランジュ的視点とは、流体の流れに沿って動く観測点を@<b>{浮かべ}、その観測点での流体の動きを観測するものとなります(@<img>{lagrange}参照)。
基本的に、オイラー的視点を用いた流体シミュレーション手法のことを格子法、ラグランジュ的視点を用いた流体シミュレーション手法のことを粒子法と呼びます。

//image[lagrange][左:オイラー的、右:ラグランジュ的][scale=0.7]{
//}

=== ラグランジュ微分(物質微分)
オイラー的視点とラグランジュ的視点では、微分の演算の仕方が異なります。
はじめに、オイラー的視点で表された物理量@<fn>{quantity}を以下に示してみます。
//footnote[quantity][物理量とは、観測できる速度や質量などのことを指します。 端的には「単位が有るもの」と捉えて良いでしょう。]
//texequation{
  \phi = \phi (\overrightarrow{x}, t)
//}
これは、時刻@<m>{t}で位置@<m>{\overrightarrow{x\}}にある物理量@<m>{\phi}という意味になります。
この物理量の時間微分は、
//texequation{
  \frac{\partial \phi}{\partial t}
//}
と表せます。
もちろんこれは、物理量の位置が@<m>{\overrightarrow{x\}}で固定されていますので、オイラー的視点での微分になります。

//footnote[advect][流れに沿った観測点の移動のことを、移流と呼びます。]
一方、ラグランジュ的視点では、観測点を流れに沿って移動@<fn>{advect}させますので、観測点自体も時間の関数となっています。
そのため、初期状態で@<m>{\overrightarrow{x\}_0}にあった観測点は、時刻@<m>{t}で
//texequation{
  \overrightarrow{x}(\overrightarrow{x}_0, t)
//}
に存在します。 よって物理量の表記も
//texequation{
  \phi = \phi (\overrightarrow{x}(\overrightarrow{x}_0, t), t)
//}
となります。
微分の定義に従って、現在の物理量と@<m>{\Delta t}秒後の物理量の変化量を見てみると
//texequation{
  \displaystyle \lim_{\Delta t \to 0} \frac{\phi(\overrightarrow{x}(\overrightarrow{x}_0, t + \Delta t), t + \Delta t) - \phi(\overrightarrow{x}(\overrightarrow{x}_0, t), t)}{\Delta t}
//}
//texequation{
  = \sum_i \frac{\partial \phi}{\partial x_i} \frac{\partial x_i}{\partial t} + \frac{\partial \phi}{\partial t}
//}
//texequation{
  = \left( \left( \begin{matrix}u_1\\u_2\\u_3\end{matrix} \right)
    \cdot
    \left( \begin{matrix} \frac{\partial}{\partial x_1}\\\frac{\partial}{\partial x_2}\\\frac{\partial}{\partial x_3} \end{matrix} \right)
    + \frac{\partial}{\partial t}
    \right) \phi\\
//}
//texequation{
  = (\frac{\partial}{\partial t} + \overrightarrow{u} \cdot {grad}) \phi
//}
となります。
これが、観測点の移動を考慮した物理量の時間微分となります。
しかしながら、この表記を用いていては式が複雑になりますので、
//texequation{
  \dfrac{D}{Dt} := \frac{\partial}{\partial t} + \overrightarrow{u} \cdot {grad}
//}
という演算子を導入することで、短く表すことができます。
これら、観測点の移動を考慮した一連の操作を、ラグランジュ微分と呼びます。
一見複雑そうに見えますが、観測点が移動する粒子法では、ラグランジュ的視点で式を表した方が都合が良くなります。

=== 流体の非圧縮条件
流体は、流体の速度が音速よりも十分に小さい場合、体積の変化が起きないとみなすことができます。
これは流体の非圧縮条件と呼ばれ、以下の数式で表されます。
//texequation{
  \nabla \cdot \overrightarrow{u} = 0
//}
これは、流体内で湧き出しや消失がないことを示しています。
この式の導出には少し複雑な積分が入りますので、説明は割愛@<fn>{bridson}します。
「流体は圧縮しない！」程度に捉えておいてください。

//footnote[bridson]["Fluid Simulation for Computer Graphics - Robert Bridson" で詳しく解説されています。]

== 粒子法シミュレーション
粒子法では、流体を小さな@<b>{粒子}によって分割し、ラグランジュ的視点で流体の動きを観測します。
この粒子が、前節の観測点にあたります。 一口に「粒子法」といっても、現在では多くの手法が提案されており、有名なものとして

 * Smoothed Particle Hydrodynamics(SPH)法
 * Fluid Implicit Particle (FLIP) 法
 * Particle In Cell (PIC) 法
 * Moving Particle Semi-implicit (MPS) 法
 * Material Point Method (MPM) 法
などがあります。

=== 粒子法におけるナビエ・ストークス方程式の導出
はじめに、粒子法におけるナビエ・ストークス方程式(以下NS方程式)は、以下のように記述されます。
//texequation{
  \dfrac{D \overrightarrow{u}}{Dt} = -\dfrac{1}{\rho}\nabla p + \nu \nabla \cdot \nabla \overrightarrow{u} + \overrightarrow{g}
  \label{eq:navier}
//}
前章の格子法で出てきたNS方程式とは少し形が異なりますね。
移流項がまるまる抜けてしまっていますが、先程のオイラー微分とラグランジュ微分の関係を見てみると、うまくこの形に変形できることがわかります。
粒子法では観測点を流れに沿って移動させますから、NS方程式計算時に移流項を考慮する必要がありません。
移流の計算はNS方程式で算出した加速度をもとに粒子位置を直接更新することで済ませる事ができます。

#@#また、NS方程式は、意外にもニュートンの第二法則である@<m>{m\overrightarrow{a\} = \overrightarrow{f\}}を変形しただけのものなのです。

現実の流体は分子の集まりですので、ある種のパーティクルシステムであると言うことができます。
しかし、コンピュータで実際の分子の数の計算を行うのは不可能ですので、計算可能な大きさに調節してあげる必要があります。
@<img>{blob}に示されているそれぞれの粒(@<fn>{blobfoot})は、計算可能な大きさで分割した流体の一部分を表していています。
これらの粒は、それぞれ質量@<m>{m}、位置ベクトル@<m>{\overrightarrow{x\}}、
速度ベクトル@<m>{\overrightarrow{u\}}、体積@<m>{V}を持つと考えることができます。
#@# 画像の位置ベクトルの表記をxに変える
//image[blob][流体のパーティクル近似][scale=0.7]{
//}
これらそれぞれの粒について、外から受けた力@<m>{\overrightarrow{f\}}を計算し、運動方程式@<m>{m \overrightarrow{a\} = \overrightarrow{f\}}を解くことで加速度が算出され、
次のタイムステップでどのように移動するかを決めることができます。

//footnote[blobfoot][英語では'Blob'と呼ばれます]

前述の通り、それぞれの粒子は周りから何らかの力を受けて動きますが、その「力」とは一体何でしょうか。
簡単な例として、重力@<m>{m \overrightarrow{g\}}があげられますが、それ以外に周りの粒子からも何らかの力を受けるはずです。
これらの力について、以下に解説します。

==== 圧力
流体粒子にかかる力の1つ目は、圧力です。
流体は必ず圧力の高い方から低い方に向かって流れます。
もし圧力がどの方向からも同じだけかかっていたとすると、力は打ち消されて動きが止まってしまいますから、圧力のバランスが不均一である場合を考えます。
前章で述べられたように、圧力のスカラー場の勾配を取ることで、自分の粒子位置から見て最も圧力上昇率の高い方向を算出することができます。
粒子が力を受ける方向は、圧力の高い方から低い方ですので、マイナスを取って@<m>{-\nabla p}となります。
また、粒子は体積を持っていますから、粒子にかかる圧力は、@<m>{-\nabla p}に粒子の体積をかけて算出します@<fn>{vol}。
最終的に、@<m>{- V \nabla p}という結果が導出されます。

//footnote[vol][流体の非圧縮条件により、単に体積をかけるだけで粒子にかかる圧力の積分を表すことができます。]

==== 粘性力
流体粒子にかかる力の２つ目は、粘性力です。
粘性(ねばりけ)のある流体とは、はちみつや溶かしたチョコレートなどに代表される、変形しづらい流体のことを指します。
粘性があるという言葉を粒子法の表現に当てはめてみると、
@<b>{粒子の速度は、周りの粒子速度の平均をとりやすい}ということになります。
前章で述べられた通り、周囲の平均をとるという演算は、ラプラシアンを用いて行うことができます。

粘性の度合いを@<b>{動粘性係数}@<m>{\mu}を用いて表すと、@<m>{\mu \nabla \cdot \nabla \overrightarrow{u\}}と表す事ができます。

==== 圧力・粘性力・外力の統合
これらの力を運動方程式@<m>{m \overrightarrow{a\} = \overrightarrow{f\}}に当てはめて整理すると、
//texequation{
  m \dfrac{D\overrightarrow{u}}{Dt} = - V \nabla p + V \mu \nabla \cdot \nabla \overrightarrow{u} + m\overrightarrow{g}
//}
ここで、@<m>{m}は@<m>{\rho V}であることから、変形して(@<m>{V}が打ち消されます)
//texequation{
  \rho \dfrac{D\overrightarrow{u}}{Dt} = - \nabla p + \mu \nabla \cdot \nabla \overrightarrow{u} + \rho \overrightarrow{g}
//}
両辺@<m>{\rho}で割り、
//texequation{
  \dfrac{D\overrightarrow{u}}{Dt} = - \dfrac{1}{\rho}\nabla p + \dfrac{\mu}{\rho} \nabla \cdot \nabla \overrightarrow{u} + \overrightarrow{g}
//}
最後に、粘性項の係数@<m>{\dfrac{\mu\}{\rho\}}に@<m>{\nu}を導入して、
//texequation{
  \dfrac{D\overrightarrow{u}}{Dt} = - \dfrac{1}{\rho}\nabla p + \nu \nabla \cdot \nabla \overrightarrow{u} + \overrightarrow{g}
//}
となり、はじめに挙げたNS方程式を導出することができました。

=== 粒子法における移流の表現
粒子法では、粒子自体が流体の観測点を表現しているので、移流項の計算は単に粒子位置を移動させるだけで完了します。
実際の時間微分の計算では、無限に小さい時間を用いますが、
コンピューターでの計算では無限を表現できないため、十分小さい時間@<m>{\Delta t}を用いて微分を表現します。
これを差分と言い、@<m>{\Delta t}を小さくすればするほど、正確な計算を行うことができます。

加速度について、差分の表現を導入すると、
//texequation{
  \overrightarrow{a} = \dfrac{D\overrightarrow{u}}{Dt} \equiv \frac{\Delta \overrightarrow{u}}{\Delta t}
//}
となります。
よって速度の増分@<m>{\Delta \overrightarrow{u\}}は、
//texequation{
\Delta \overrightarrow{u} = \Delta t \overrightarrow{a}
//}
となり、また、位置の増分についても同様に、
//texequation{
  \overrightarrow{u} = \frac{\partial \overrightarrow{x}}{\partial t} \equiv \frac{\Delta \overrightarrow{x}}{\Delta t}
//}
より、
//texequation{
\Delta \overrightarrow{x} = \Delta t \overrightarrow{u}
//}
となります。

この結果を利用することで、次のフレームでの速度ベクトルと位置ベクトルを算出できます。
現在のフレームでの粒子速度が@<m>{\overrightarrow{u\}_n}であるとすると、
次のフレームでの粒子速度は@<m>{\overrightarrow{u\}_{n+1\}}で、
//texequation{
\overrightarrow{u}_{n+1} = \overrightarrow{u}_n + \Delta \overrightarrow{u} = \overrightarrow{u}_n + \Delta t \overrightarrow{a}
//}
と表せます。

現在のフレームでの粒子位置が@<m>{\overrightarrow{x\}_n}であるとすると、
次のフレームでの粒子位置は@<m>{\overrightarrow{x\}_{n+1\}}で、
//texequation{
\overrightarrow{x}_{n+1} = \overrightarrow{x}_n + \Delta \overrightarrow{x} = \overrightarrow{x}_n + \Delta t \overrightarrow{u}
//}
と表せます。

この手法は、前進オイラー法と呼ばれます。
これを毎フレーム繰り返すことで、各時刻での粒子の移動を表現することができます。


== SPH法による流体シミュレーション
前節では、粒子法におけるNS方程式の導出方法について解説しました。
もちろん、これらの微分方程式をコンピュータでそのまま解くことはできませんので、何らかの近似をしてあげる必要があります。
その手法として、CG分野でよく用いられる@<b>{SPH法}について解説します。

SPH法は、本来宇宙物理学における天体同士の衝突シミュレーションに用いられていた手法ですが、1996年にDesbrunら@<fn>{desbrun}によってCGにおける流体シミュレーションにも応用されました。
また、並列化も容易で、現在のGPUでは大量の粒子の計算をリアルタイムに行うことが可能です。
コンピュータシミュレーションでは、連続的な物理量を離散化して計算を行う必要がありますが、
この離散化を、@<b>{重み関数}と呼ばれる関数を用いて行う手法をSPH法と呼びます。

//footnote[desbrun][Desbrun and Cani, Smoothed Particles: A new paradigm for animating highly deformable bodies, Eurographics Workshop on Computer Animation and Simulation (EGCAS), 1996.]

=== 物理量の離散化

SPH法では、粒子一つ一つが影響範囲を持っていて、他の粒子と距離が近いほどその粒子の影響が大きく受けるという動作をします。
この影響範囲を図示すると@<img>{2dkernel}のようになります。
//image[2dkernel][2次元の重み関数][scale=0.5]{
//}
この関数を@<b>{重み関数}@<fn>{weight_fn}と呼びます。

//footnote[weight_fn][通常この関数はカーネル関数とも呼ばれますが、ComputeShaderにおけるカーネル関数と区別するためこの呼び方にしています。]

SPH法における物理量を@<m>{\phi}とすると、重み関数を用いて以下のように離散化されます。
//texequation{
  \phi(\overrightarrow{x}) = \sum_{j \in N}m_j\frac{\phi_j}{\rho_j}W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
@<m>{N, m, \rho, h}はそれぞれ、近傍粒子の集合、粒子の質量、粒子の密度、重み関数の影響半径です。
また、関数@<m>{W}が先ほど述べた重み関数になります。

さらに、この物理量には、勾配とラプラシアンなどの偏微分演算が適用でき、
勾配は、
//texequation{
  \nabla \phi(\overrightarrow{x}) = \sum_{j \in N}m_j\frac{\phi_j}{\rho_j} \nabla W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
ラプラシアンは、
//texequation{
  \nabla^2 \phi(\overrightarrow{x}) = \sum_{j \in N}m_j\frac{\phi_j}{\rho_j} \nabla^2 W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
と表せます。
式からわかるように、物理量の勾配及びラプラシアンは、重み関数に対してのみ適用されるイメージになります。
重み関数@<m>{W}は、求めたい物理量によって異なるものを使用しまが、この理由の説明については割愛@<fn>{fujisawa}します。
#@#重み関数のグラフ画像
//footnote[fujisawa]["CGのための物理シミュレーションの基礎 - 藤澤誠" で詳しく解説されています。]

=== 密度の離散化
流体の粒子の密度は、先ほどの重み関数で離散化した物理量の式を利用して、
//texequation{
  \rho(\overrightarrow{x}) = \sum_{j \in N}m_jW_{poly6}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
と与えられます。
ここで、利用する重み関数@<m>{W}は、以下で与えられます。
//image[poly6][Poly6重み関数][scale=0.7]{
//}

=== 粘性項の離散化
粘性項を離散化も密度の場合と同様重み関数を利用して、
//texequation{
  f_{i}^{visc} = \mu\nabla^2\overrightarrow{u}_i = \mu \sum_{j \in N}m_j\frac{\overrightarrow{u}_j - \overrightarrow{u}_i}{\rho_j} \nabla^2 W_{visc}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
と表されます。
ここで、重み関数のラプラシアン@<m>{\nabla^2 W_{visc\}}は、以下で与えられます。
//image[visc][Viscosity重み関数のラプラシアン][scale=0.7]{
//}

=== 圧力項の離散化
同様に、圧力項を離散化していきます。
//texequation{
  f_{i}^{press} = - \frac{1}{\rho_i} \nabla p_i = - \frac{1}{\rho_i} \sum_{j \in N}m_j\frac{p_j - p_i}{2\rho_j} \nabla W_{spiky}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
ここで、重み関数の勾配@<m>{W_{spiky\}}は以下で与えられます。
//image[spiky][Spiky重み関数の勾配][scale=0.7]{
//}

この時、粒子の圧力は事前に、Tait方程式と呼ばれる、
//texequation{
    p = B\left\{\left(\frac{\rho}{\rho_0}\right)^\gamma - 1\right\}
//}
で算出されています。 ここで、@<m>{B}は気体定数です。
非圧縮性を保証するためには、本来ポアソン方程式を解かなければならないのですが、リアルタイム計算には向きません。
その代わりSPH法@<fn>{wcsph}では、近似的に非圧縮性を確保する点で格子法よりも圧力項の計算が苦手であるといわれます。

//footnote[wcsph][Tait方程式を用いた圧力計算を行うSPH法を、特別にWCSPH法と呼びます。]

== SPH法の実装
サンプルはこちらのリポジトリ(@<href>{https://github.com/IndieVisualLab/UnityGraphicsProgramming})のAssets/SPHFluid以下に掲載しています。
今回の実装では、極力シンプルにSPHの手法を解説するために高速化や数値安定性は考慮していませんのでご了承ください。

=== パラメータ
シミュレーションに使用する諸々のパラメータの説明については、コード内コメントに記載しています。
//listnum[parameters][シミュレーションに使用するパラメータ(FluidBase.cs)][csharp]{
NumParticleEnum particleNum = NumParticleEnum.NUM_8K;    // 粒子数
float smoothlen = 0.012f;               // 粒子半径
float pressureStiffness = 200.0f;       // 圧力項係数
float restDensity = 1000.0f;            // 静止密度
float particleMass = 0.0002f;           // 粒子質量
float viscosity = 0.1f;                 // 粘性係数
float maxAllowableTimestep = 0.005f;    // 時間刻み幅
float wallStiffness = 3000.0f;          // ペナルティ法の壁の力
int iterations = 4;                     // イテレーション回数
Vector2 gravity = new Vector2(0.0f, -0.5f);     // 重力
Vector2 range = new Vector2(1, 1);              // シミュレーション空間
bool simulate = true;                           // 実行 or 一時停止

int numParticles;              // パーティクルの個数
float timeStep;                // 時間刻み幅
float densityCoef;             // Poly6カーネルの密度係数
float gradPressureCoef;        // Spikyカーネルの圧力係数
float lapViscosityCoef;        // Laplacianカーネルの粘性係数
//}

今回のデモシーンでは、コードに記載されているパラメータの初期化値とは異なる値をインスペクタで設定していますので注意してください。

=== SPH重み関数の係数の計算
重み関数の係数はシミュレーション中で変化しないため、初期化時にCPU側で計算しておきます。
(ただし、実行途中でパラメータを編集する可能性も踏まえてUpdate関数内で更新しています)

今回、粒子ごとの質量はすべて一定にしているので、物理量の式内にある質量@<m>{m}はシグマの外に出て以下になります。
//texequation{
  \phi(\overrightarrow{x}) = m \sum_{j \in N}\frac{\phi_j}{\rho_j}W(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
そのため、係数計算の中に質量を含めてしまうことができます。

重み関数の種類で係数も変化してきますから、それぞれに関して係数を計算します。

//listnum[coefs][重み関数の係数の事前計算(FluidBase.cs)][csharp]{
densityCoef = particleMass * 4f / (Mathf.PI * Mathf.Pow(smoothlen, 8));
gradPressureCoef
    = particleMass * -30.0f / (Mathf.PI * Mathf.Pow(smoothlen, 5));
lapViscosityCoef
    = particleMass * 20f / (3 * Mathf.PI * Mathf.Pow(smoothlen, 5));
//}

最終的に、これらのCPU側で計算した係数(及び各種パラメータ)をGPU側の定数バッファに格納します。
//listnum[setconst][ComputeShaderの定数バッファに値を転送する(FluidBase.cs)][csharp]{
fluidCS.SetInt("_NumParticles", numParticles);
fluidCS.SetFloat("_TimeStep", timeStep);
fluidCS.SetFloat("_Smoothlen", smoothlen);
fluidCS.SetFloat("_PressureStiffness", pressureStiffness);
fluidCS.SetFloat("_RestDensity", restDensity);
fluidCS.SetFloat("_Viscosity", viscosity);
fluidCS.SetFloat("_DensityCoef", densityCoef);
fluidCS.SetFloat("_GradPressureCoef", gradPressureCoef);
fluidCS.SetFloat("_LapViscosityCoef", lapViscosityCoef);
fluidCS.SetFloat("_WallStiffness", wallStiffness);
fluidCS.SetVector("_Range", range);
fluidCS.SetVector("_Gravity", gravity);
//}

//listnum[const][ComputeShaderの定数バッファ(SPH2D.compute)][csharp]{
int   _NumParticles;      // 粒子数
float _TimeStep;          // 時間刻み幅(dt)
float _Smoothlen;         // 粒子半径
float _PressureStiffness; // Beckerの係数
float _RestDensity;       // 静止密度
float _DensityCoef;       // 密度算出時の係数
float _GradPressureCoef;  // 圧力算出時の係数
float _LapViscosityCoef;  // 粘性算出時の係数
float _WallStiffness;     // ペナルティ法の押し返す力
float _Viscosity;         // 粘性係数
float2 _Gravity;          // 重力
float2 _Range;            // シミュレーション空間

float3 _MousePos;         // マウス位置
float _MouseRadius;       // マウスインタラクションの半径
bool _MouseDown;          // マウスが押されているか
//}


=== 密度の計算
//listnum[density_kernel][密度の計算を行うカーネル関数(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void DensityCS(uint3 DTid : SV_DispatchThreadID) {
	uint P_ID = DTid.x;	// 現在処理しているパーティクルID

	float h_sq = _Smoothlen * _Smoothlen;
	float2 P_position = _ParticlesBufferRead[P_ID].position;

	// 近傍探索(O(n^2))
	float density = 0;
	for (uint N_ID = 0; N_ID < _NumParticles; N_ID++) {
		if (N_ID == P_ID) continue;	// 自身の参照回避

		float2 N_position = _ParticlesBufferRead[N_ID].position;

		float2 diff = N_position - P_position;    // 粒子距離
		float r_sq = dot(diff, diff);             // 粒子距離の2乗

		// 半径内に収まっていない粒子は除外
		if (r_sq < h_sq) {
            // 計算には2乗しか含まれないのでルートをとる必要なし
			density += CalculateDensity(r_sq);
		}
	}

	// 密度バッファを更新
	_ParticlesDensityBufferWrite[P_ID].density = density;
}
//}

本来であれば粒子を全数調査せず、適切な近傍探索アルゴリズムを用いて近傍粒子を探す必要がありますが、
今回の実装では簡単のために全数調査を行っています(10行目のforループ)。
また、自分と相手粒子との距離計算を行うため、11行目で自身の粒子同士で計算を行うのを回避しています。

重み関数の有効半径@<m>{h}による場合分けは19行目のif文で実現します。
密度の足し合わせ(シグマの計算)は、9行目で0で初期化しておいた変数に対してシグマ内部の計算結果を加算していくことで実現します。
ここで、もう一度密度の計算式を示します。
//texequation{
  \rho(\overrightarrow{x}) = \sum_{j \in N}m_jW_{poly6}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
密度の計算は上式のとおり、Poly6重み関数を用います。 Poly6重み関数は@<list>{density_weight}で計算します。
//listnum[density_weight][密度の計算(SPH2D.compute)][c]{
inline float CalculateDensity(float r_sq) {
	const float h_sq = _Smoothlen * _Smoothlen;
	return _DensityCoef * (h_sq - r_sq) * (h_sq - r_sq) * (h_sq - r_sq);
}
//}

最終的に@<list>{density_kernel}の25行目で書き込み用バッファに書き込みます。

=== 粒子単位の圧力の計算
//listnum[press_kernel][粒子毎の圧力を計算する重み関数(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void PressureCS(uint3 DTid : SV_DispatchThreadID) {
	uint P_ID = DTid.x;	// 現在処理しているパーティクルID

	float  P_density = _ParticlesDensityBufferRead[P_ID].density;
	float  P_pressure = CalculatePressure(P_density);

	// 圧力バッファを更新
	_ParticlesPressureBufferWrite[P_ID].pressure = P_pressure;
}
//}

圧力項を解く前に、粒子単位の圧力を算出しておき、後の圧力項の計算コストを下げます。
先程も述べましたが、圧力の計算では本来、以下の式のようなポアソン方程式と呼ばれる方程式を解く必要があります。
//texequation{
    \nabla^2 p = \rho \frac{\nabla \overrightarrow{u}}{\Delta t}
//}
しかし、コンピュータで正確にポアソン方程式を解く操作は非常に計算コストが高いため、以下のTait方程式を用いて近似的に求めます。
//texequation{
    p = B\left\{\left(\frac{\rho}{\rho_0}\right)^\gamma - 1\right\}
//}
//listnum[tait][Tait方程式の実装(SPH2D.compute)][c]{
inline float CalculatePressure(float density) {
	return _PressureStiffness * max(pow(density / _RestDensity, 7) - 1, 0);
}
//}


=== 圧力項・粘性項の計算
//listnum[force_kernel][圧力項・粘性項を計算するカーネル関数(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void ForceCS(uint3 DTid : SV_DispatchThreadID) {
	uint P_ID = DTid.x; // 現在処理しているパーティクルID

	float2 P_position = _ParticlesBufferRead[P_ID].position;
	float2 P_velocity = _ParticlesBufferRead[P_ID].velocity;
	float  P_density = _ParticlesDensityBufferRead[P_ID].density;
	float  P_pressure = _ParticlesPressureBufferRead[P_ID].pressure;

	const float h_sq = _Smoothlen * _Smoothlen;

	// 近傍探索(O(n^2))
	float2 press = float2(0, 0);
	float2 visco = float2(0, 0);
	for (uint N_ID = 0; N_ID < _NumParticles; N_ID++) {
		if (N_ID == P_ID) continue;	// 自身を対象とした場合スキップ

		float2 N_position = _ParticlesBufferRead[N_ID].position;

		float2 diff = N_position - P_position;
		float r_sq = dot(diff, diff);

		// 半径内に収まっていない粒子は除外
		if (r_sq < h_sq) {
			float  N_density
                    = _ParticlesDensityBufferRead[N_ID].density;
			float  N_pressure
                    = _ParticlesPressureBufferRead[N_ID].pressure;
			float2 N_velocity
                    = _ParticlesBufferRead[N_ID].velocity;
			float  r = sqrt(r_sq);

			// 圧力項
			press += CalculateGradPressure(...);

			// 粘性項
			visco += CalculateLapVelocity(...);
		}
	}

	// 統合
	float2 force = press + _Viscosity * visco;

	// 加速度バッファの更新
	_ParticlesForceBufferWrite[P_ID].acceleration = force / P_density;
}
//}

圧力項、粘性項の計算も、密度の計算方法と同様に行います。

初めに、以下の圧力項による力の計算を31行目にて行っています。
//texequation{
  f_{i}^{press} = - \frac{1}{\rho_i} \nabla p_i = - \frac{1}{\rho_i} \sum_{j \in N}m_j\frac{p_j - p_i}{2\rho_j} \nabla W_{press}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
シグマの中身の計算は以下の関数で行われます。
//listnum[press_weight][圧力項の要素の計算(SPH2D.compute)][c]{
inline float2 CalculateGradPressure(...) {
	const float h = _Smoothlen;
	float avg_pressure = 0.5f * (N_pressure + P_pressure);
	return _GradPressureCoef * avg_pressure / N_density
            * pow(h - r, 2) / r * (diff);
}
//}

次に、以下の粘性項による力の計算を34行目で行っています。
//texequation{
  f_{i}^{visc} = \mu\nabla^2\overrightarrow{u}_i = \mu \sum_{j \in N}m_j\frac{\overrightarrow{u}_j - \overrightarrow{u}_i}{\rho_j} \nabla^2 W_{visc}(\overrightarrow{x_j} - \overrightarrow{x}, h)
//}
シグマの中身の計算は以下の関数で行われます。
//listnum[visc_weight][粘性項の要素の計算(SPH2D.compute)][c]{
inline float2 CalculateLapVelocity(...) {
	const float h = _Smoothlen;
	float2 vel_diff = (N_velocity - P_velocity);
	return _LapViscosityCoef / N_density * (h - r) * vel_diff;
}
//}

最後に、@<list>{force_kernel}の39行目にて圧力項と粘性項で算出した力を足し合わせ、最終的な出力としてバッファに書き込んでいます。

=== 衝突判定と位置更新
//listnum[integrate_kernel][衝突判定と位置更新を行うカーネル関数(SPH2D.compute)][c]{
[numthreads(THREAD_SIZE_X, 1, 1)]
void IntegrateCS(uint3 DTid : SV_DispatchThreadID) {
	const unsigned int P_ID = DTid.x; // 現在処理しているパーティクルID

	// 更新前の位置と速度
	float2 position = _ParticlesBufferRead[P_ID].position;
	float2 velocity = _ParticlesBufferRead[P_ID].velocity;
	float2 acceleration = _ParticlesForceBufferRead[P_ID].acceleration;

	// マウスインタラクション
	if (distance(position, _MousePos.xy) < _MouseRadius && _MouseDown) {
		float2 dir = position - _MousePos.xy;
		float pushBack = _MouseRadius-length(dir);
		acceleration += 100 * pushBack * normalize(dir);
	}

	// 衝突判定を書くならここ -----

	// 壁境界(ペナルティ法)
	float dist = dot(float3(position, 1), float3(1, 0, 0));
	acceleration += min(dist, 0) * -_WallStiffness * float2(1, 0);

	dist = dot(float3(position, 1), float3(0, 1, 0));
	acceleration += min(dist, 0) * -_WallStiffness * float2(0, 1);

	dist = dot(float3(position, 1), float3(-1, 0, _Range.x));
	acceleration += min(dist, 0) * -_WallStiffness * float2(-1, 0);

	dist = dot(float3(position, 1), float3(0, -1, _Range.y));
	acceleration += min(dist, 0) * -_WallStiffness * float2(0, -1);

	// 重力の加算
	acceleration += _Gravity;

	// 前進オイラー法で次の粒子位置を更新
	velocity += _TimeStep * acceleration;
	position += _TimeStep * velocity;

	// パーティクルのバッファ更新
	_ParticlesBufferWrite[P_ID].position = position;
	_ParticlesBufferWrite[P_ID].velocity = velocity;
}
//}

壁との衝突判定をペナルティ法を用いて行います(19-30行目)。
ペナルティ法とは、境界位置からはみ出した分だけ強い力で押し返すという手法になります。

本来は壁との衝突判定の前に障害物との衝突判定も行うのですが、今回の実装ではマウスとのインタラクションを行うようにしています(213-218行目)。
マウスが押されていれば、指定された力でマウス位置から遠ざかるような力を加えています。

33行目にて外力である重力を加算しています。
重力の値をゼロにすると無重力状態になり、面白い視覚効果が得られます。
また、位置の更新は前述の前進オイラー法で行い(36-37行目)、最終的な結果をバッファに書き込みます。


=== シミュレーションメインルーチン
//listnum[routine][シミュレーションの主要関数(FluidBase.cs)][csharp]{
private void RunFluidSolver() {

  int kernelID = -1;
  int threadGroupsX = numParticles / THREAD_SIZE_X;

  // Density
  kernelID = fluidCS.FindKernel("DensityCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  // Pressure
  kernelID = fluidCS.FindKernel("PressureCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesPressureBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  // Force
  kernelID = fluidCS.FindKernel("ForceCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesDensityBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesPressureBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesForceBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  // Integrate
  kernelID = fluidCS.FindKernel("IntegrateCS");
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesForceBufferRead", ...);
  fluidCS.SetBuffer(kernelID, "_ParticlesBufferWrite", ...);
  fluidCS.Dispatch(kernelID, threadGroupsX, 1, 1);

  SwapComputeBuffer(ref particlesBufferRead, ref particlesBufferWrite);
}
//}
これまでに述べたComputeShaderのカーネル関数を、毎フレーム呼び出す部分です。
それぞれのカーネル関数に対して適切なComputeBufferを与えてあげます。

ここで、タイムステップ幅@<m>{\Delta t}を小さくすればするほどシミュレーションの誤差が出にくくなることを思い出してみてください。
60FPSで実行する場合、@<m>{\Delta t = 1 / 60}となりますが、これでは誤差が大きく出てしまい粒子が爆発してしまいます。
さらに、@<m>{\Delta t = 1 / 60}より小さいタイムステップ幅をとると、1フレーム当たりの時間の進み方が実時間より遅くなり、スローモーションになってしまいます。
これを回避するには、@<m>{\Delta t = 1 / (60 \times {iterarion\})}として、メインルーチンを1フレームにつきiterarion回回します。

//listnum[iteration][主要関数のイテレーション(FluidBase.cs)][csharp]{
// 計算精度を上げるために時間刻み幅を小さくして複数回イテレーションする
for (int i = 0; i<iterations; i++) {
    RunFluidSolver();
}
//}
こうすることで、小さいタイムステップ幅で実時間のシミュレーションを行うことができます。

=== バッファの使い方
通常のシングルアクセスのパーティクルシステムとは異なり、
粒子同士が相互作用しますから、計算途中に他のデータが書き換わってしまっては困ります。
これを回避するために、GPUで計算を行っている際に値を書き換えない読み込み用バッファと書き込み用バッファの2つを用意します。
これらのバッファを毎フレーム入れ替えることで、競合なくデータを更新できます。
//listnum[swap][バッファを入れ替える関数(FluidBase.cs)][csharp]{
void SwapComputeBuffer(ref ComputeBuffer ping, ref ComputeBuffer pong) {
    ComputeBuffer temp = ping;
    ping = pong;
    pong = temp;
}
//}

=== 粒子のレンダリング
//listnum[rendercs][パーティクルのレンダリング(FluidRenderer.cs)][csharp]{
void DrawParticle() {

  Material m = RenderParticleMat;

  var inverseViewMatrix = Camera.main.worldToCameraMatrix.inverse;

  m.SetPass(0);
  m.SetMatrix("_InverseMatrix", inverseViewMatrix);
  m.SetColor("_WaterColor", WaterColor);
  m.SetBuffer("_ParticlesBuffer", solver.ParticlesBufferRead);
  Graphics.DrawProcedural(MeshTopology.Points, solver.NumParticles);
}
//}
10行目にて、流体粒子の位置計算結果を格納したバッファをマテリアルにセットし、シェーダーに転送します。
11行目にて、パーティクルの個数分インスタンス描画をするよう命令しています。

//listnum[render][パーティクルのレンダリング(Particle.shader)][c]{
struct FluidParticle {
	float2 position;
	float2 velocity;
};

StructuredBuffer<FluidParticle> _ParticlesBuffer;

// --------------------------------------------------------------------
// Vertex Shader
// --------------------------------------------------------------------
v2g vert(uint id : SV_VertexID) {

	v2g o = (v2g)0;
	o.pos = float3(_ParticlesBuffer[id].position.xy, 0);
	o.color = float4(0, 0.1, 0.1, 1);
	return o;
}
//}
1-6行目にて、流体粒子の情報を受け取るための情報の定義を行います。
この時、スクリプトからマテリアルに転送したバッファの構造体と定義を一致させる必要があります。
位置データの受け取りは、14行目のようにid : SV_VertexIDでバッファの要素を参照することで行います。

あとは通常のパーティクルシステムと同様、@<img>{bill}のように
ジオメトリシェーダーで計算結果の位置データを中心としたビルボード@<fn>{billboard}を作成し、
粒子画像をアタッチしてレンダリングします。
//image[bill][ビルボードの作成][scale=1]{
//}

//footnote[billboard][表が常に視点方向を向くPlaneのことを指します。]

== 結果
//image[result][レンダリング結果]{
//}

動画はこちら(@<href>{https://youtu.be/KJVu26zeK2w})に掲載しています。

== まとめ
本章では、SPH法を用いた流体シミュレーションの手法を示しました。
SPH法を用いることで、流体の動きをパーティクルシステムのように汎用的に扱うことができるようになりました。

先述の通り、流体シミュレーションの手法はSPH法以外にもたくさんの種類があります。
本章を通して、他の流体シミュレーション手法に加え、他の物理シミュレーション自体についても興味を持っていただき、
表現の幅を広げていただければ幸いです。
