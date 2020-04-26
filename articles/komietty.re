
= MCMCで行う３次元空間サンプリング

== はじめに


本章ではサンプリング手法について解説していきます。今回取り上げるのは、ある確率分布の中から適当な値を複数サンプリングしてくるMCMC（マルコフ連鎖モンテカルロ法）というサンプリング方法です。



ある確率分布からサンプリングしてくる方法として最も簡単な方法に棄却法という方法がありますが、３次元空間でのサンプリングでは棄却される領域が大きく実際の運用に耐えません。そこでMCMCを使うことで高次元においても効率よくサンプリングできるというのが、本章の内容です。



MCMCに関する情報は、一方では書籍など体系だった情報は統計屋さん向けのものでプログラマにとっては冗長な割に実装までの手引が存在せず、他方ネットにある情報は１０数行のサンプルコードが記載されているだけで理論的な背景へのケアがないため、理論と実装を手早く一気通貫に理解できるコンテンツが存在しないのが実情です。次節以降の具体的な解説はできるだけそういった内容になるように心がけました。



MCMCの背景となる確率の解説は、厳密を期せばそれこそ本が一冊書けるほどの内容です。今回は安心して実装できる最小限の理論的背景の説明をモットーに、定義の厳密性は程々に、なるだけ直感的な表現を目指しました。数学については大学初年度程度、プログラムについては仕事で少しでも使ったことがある程度の方なら難なく読める内容かなと思います。



== サンプルリポジトリ


本章ではUnityGraphicsProgrammingのUnityプロジェクトhttps://github.com/IndieVisualLab/UnityGraphicsProgramming内にあるAssets/ProceduralModeling以下をサンプルプログラムとして用意しています。



== 確率に関する基礎知識


MCMCの理論を理解するには、まずは確率についての基礎的な内容を抑えておく必要があります。
ただし今回MCMCを理解するために押さえておくべき概念は少なく、以下の４つだけです。尤度も確率密度関数も必要なしです！

 * 確率変数
 * 確率分布
 * 確率過程
 * 定常分布



順に見ていきましょう。


=== 確率変数


ある事象が確立 P(X) で起こるときの、この実数Xを確率変数と呼びます。例えば「サイコロの５の目が出る確率は1/6である」という時に「５の目」が確率変数にあたり「1/6」が確率に当たります。先程の文を一般的に言い換えると「サイコロのXの目がでる確率はP(X)である」と言い換えることができます。



ちなみにすこし定義らしい書き方をすると、確率変数Xは標本空間Ω（＝起こる可能性のある全ての事象）から選ばれた元ω（＝起こった一つの事象）について、実数であるXを返す写像 X = X(ω) と書くことができます。


=== 確率過程


先程の確率変数の後半で若干ややこしい定義を付け加えたのは、確率変数Xが X = X(ω) という書き方で表されるという前提に立つと、確率過程の理解が簡単になるからです。確率過程とは、先程のXに時間の条件を付け加えたもので X = X(ω, t) と表すことができるもののこと。つまり確率過程は時間の条件を添えた確率変数の一種と考えることができます。


=== 確率分布


確率分布は、確率変数 X と 確率 P(X) との対応関係を示すものです。よく縦軸に確率 P(X) 横軸に X を取ったグラフで表します。


=== 定常分布


一つ一つの点は遷移しても全体の分布が不変であるような分布。分布 P とある遷移行列 π について、πP = P を満たす P を定常分布と呼びます。この定義だけではわかりにくいですが、以下の図を見れば明らかです。



//image[komiettyfig04][stationaryDistribution]{
//}



== MCMCの概念


さて本節ではMCMCを構成する概念について触れていきます。@<br>{}
MCMCは最初に述べたように、ある確率分布の中から適当な値をサンプリングしてくる手法なのですが、より具体的には、与えられた分布が定常分布であるという条件の下でモンテカルロ法(Monte Carlo)とマルコフ連鎖(Markov chain)によってサンプリングする手法を指します。以下ではモンテカルロ法、マルコフ連鎖、定常分布、の順に解説をおこなっていきます。


=== モンテカルロ法


モンテカルロ法とは、擬似乱数を使った数値計算やシミュレーションの総称です。@<br>{}
よくモンテカルロ法による数値計算の導入に使われる例に、以下のような円周率の計算があります。


//emlist{
float pi;
float trial = 10000;
float count = 0;

for(int i=0; i<trial; i++){
    float x = Random.value;
    float y = Random.value;
    if(x*x+y*y <= 1) count++;
}

pi = 4 * count / trial;
//}


要するに1 x 1の正方形の中で扇形の円の中に入った試行数と全体の試行数の比が面積比になるので、そこから円周率を出す事ができるというものです。簡単な例ですが、これもモンテカルロ法です。


=== マルコフ連鎖


マルコフ連鎖は、マルコフ性を満たす確率過程のうち、状態が離散的に記述できるものを指します。@<br>{}
マルコフ性とは、ある確率過程の将来状態の確率分布が現在状態のみに依存し、過去の状態に依存しない性質のことです。



//image[komiettyfig01][MarkovChain]{
//}




上図のようにマルコフ連鎖では将来の状態は現在の状態のみに依存して、過去の状態には直接的には影響しません。


=== 定常分布


MCMCでは擬似乱数を使ってある任意の分布から与えられた定常分布へと収束していく必要があります。というのも、与えられた分布に収束しないと毎回違う分布からサンプリングしてしまうし、定常分布でないと上手く連鎖的にサンプリングできません。任意の分布が与えられた分布へと収束するには、以下の二つの条件を満たす必要があります。

 * 既約性・・・分布が複数の部分に別れていてはいけないという条件。確率分布上のある点から遷移を繰り返していく際に、到達できない点が存在してはならない



//image[komiettyfig02][Irreducibility]{
//}


 * 非周期性・・・どんなｎに対してもｎ回で元いた場所に戻ってこれるという条件。例えば円周上に並んだ分布の中で、一つ飛ばしにしか遷移できないとった条件が存在してはならない。



//image[komiettyfig03][Aperiodicity]{
//}




この２つの条件を満たしていればある任意の分布は与えられた定常分布に収束することができます。これをマルコフ過程のエルゴード性といいます。


=== メトロポリス法


さて与えられた分布が先程のエルゴート性を満たす分布かどうかをいちいち調べるのは骨が折れることなので、多くの場合には条件を強めにとって「詳細釣り合い」という条件を満たす範囲で調べていきます。詳細釣り合いをみたすマルコフ連鎖の手法の一つがメトロポリス法と呼ばれるものです。



メトロポリス法は以下の２ステップを踏むことでサンプリングを行います

 1. 擬似乱数で遷移先の候補 x を選ぶ。x は Q(x|x') = Q(x'|x) を満たすような分布 Q に従って生成され、この分布 Q を提案分布と呼ぶ。提案分布としてガウス分布が選ばれることが多い。
 1. 1 と独立な乱数を発生させて、その乱数を使ってある基準が満たされれば遷移先候補を採用する。  具体的には、一様乱数 0 <= r < 1 に対して目標分布上の確率値 P(x) と遷移候補先の確率値 P(x') の比P(x')/P(x) が、 P(x')/P(x) > r を満たせば遷移候補先へ遷移する。



メトロポリス法のメリットは、確率分布の極大値に遷移しきった後も r の値が小さければ確率値の小さい方に遷移するので、極大値周辺で確率値に比例したサンプリングができることです。



ちなみにメトロポリス法はメトロポリス・ヘイスティング法（MH法）の一種です。メトロポリス法は提案分布に左右対称な分布を使いますが、MH法ではこの限りではありません。


== ３次元サンプリング


では実際にコードの抜粋を見ながら、どのようにMCMCを実装するかを見ていきましょう。

先ず３次元の確率分布を用意します。これを目標分布と呼びます。実際にサンプリングしたい分布なので「目標」分布です。


//emlist{
void Prepare()
{
    var sn = new SimplexNoiseGenerator();
    for (int x = 0; x < lEdge; x++)
        for (int y = 0; y < lEdge; y++)
            for (int z = 0; z < lEdge; z++)
            {
                var i = x + lEdge * y + lEdge * lEdge * z;
                var val = sn.noise(x, y, z);
                data[i] = new Vector4(x, y, z, val);
            }
}
//}


今回はシンプレックスノイズを目標分布として採用しました。



次に実際にMCMCを走らせます。


//emlist{
public IEnumerable<Vector3> Sequence(int nInit, int limit, float th)
{
    Reset();

    for (var i = 0; i < nInit; i++)
        Next(th);

    for (var i = 0; i < limit; i++)
    {
        yield return _curr;
        Next(th);
    }
}
//}

//emlist{
public void Reset()
{
     for (var i = 0; _currDensity <= 0f && i < limitResetLoopCount; i++)
     {
             _curr = new Vector3(
               Scale.x * Random.value,
               Scale.y * Random.value,
               Scale.z * Random.value
               );
             _currDensity = Density(_curr);
     }
}
//}


コルーチンを使って処理を走らせます。MCMCは一つのマルコフ連鎖が終わると全く別のところから処理が始まるため、概念的には並列処理と考えることができます。今回はReset関数を使って、一連の処理が終わった後に別の処理を走らせるようにしています。この作業を行うことで、確率分布の極大値が多数存在する場合にも上手くサンプリングができるようになります。



遷移を始めて最初の方は目標分布から離れた点である可能性が高いので、この区間はサンプリングを行わず捨ててしまします（burn-in）。十分目標分布に近づいたらサンプリングと遷移のセットを一定回数行い、終わったらまた別の一連の処理に入ります。



最後に遷移を決定する処理です。@<br>{}
３次元ですので、提案分布は以下のように三変量の標準正規分布を用います。


//emlist{
public static Vector3 GenerateRandomPointStandard()
{
        var x = RandomGenerator.rand_gaussian(0f, 1f);
        var y = RandomGenerator.rand_gaussian(0f, 1f);
        var z = RandomGenerator.rand_gaussian(0f, 1f);
        return new Vector3(x, y, z);
}
//}

//emlist{
public static float rand_gaussian(float mu, float sigma)
{
     float z = Mathf.Sqrt(-2.0f * Mathf.Log(Random.value))
              * Mathf.Sin(2.0f * Mathf.PI * Random.value);
     return mu + sigma * z;
}
//}


メトロポリス法では左右対称な分布である必要があるので、平均値を０以外に設定することは無いですが、分散を１以外にする場合は、コレスキー分解を使って以下のように導出します。


//emlist{
public static Vector3 GenerateRandomPoint(Matrix4x4 sigma)
{
    var c00 = sigma.m00 / Mathf.Sqrt(sigma.m00);
    var c10 = sigma.m10 / Mathf.Sqrt(sigma.m00);
    var c20 = sigma.m21 / Mathf.Sqrt(sigma.m00);
    var c11 = Mathf.Sqrt(sigma.m11 - c10 * c10);
    var c21 = (sigma.m21 - c20 * c10) / c11;
    var c22 = Mathf.Sqrt(sigma.m22 - (c20 * c20 + c21 * c21));
    var r1 = RandomGenerator.rand_gaussian(0f, 1f);
    var r2 = RandomGenerator.rand_gaussian(0f, 1f);
    var r3 = RandomGenerator.rand_gaussian(0f, 1f);
    var x = c00 * r1;
    var y = c10 * r1 + c11 * r2;
    var z = c20 * r1 + c21 * r2 + c22 * r3;
    return new Vector3(x, y, z);
}
//}


遷移先の決定は、提案分布（上の一点である）nextと直前の点_currそれぞれの、目標分布上における確率の比を取り一様乱数より大きければ遷移、そうでなければ遷移しない、とします。@<br>{}
確率値は、遷移先の座標に対応する確立値を見つける処理が重いため(O(n^3)の処理量)、近似計算を行っています。今回は目標分布が連続的に変化する分布を用いているので、距離に反比例する加重平均を行うことで近似的に確立値を導出しています。


//emlist{
void Next(float threshold)
{
        Vector3 next =
          GaussianDistributionCubic.GenerateRandomPointStandard()
          + _curr;

        var densityNext = Density(next);
        bool flag1 =
          _currDensity <= 0f ||
          Mathf.Min(1f, densityNext / _currDensity) >= Random.value;
        bool flag2 = densityNext > threshold;
        if (flag1 && flag2)
        {
                _curr = next;
                _currDensity = densityNext;
        }
}

float Density(Vector3 pos)
{
        float weight = 0f;
        for (int i = 0; i < weightReferenceloopCount; i++)
        {
                int id = (int)Mathf.Floor(Random.value * (Data.Length - 1));
                Vector3 posi = Data[id];
                float mag = Vector3.SqrMagnitude(pos - posi);
                weight += Mathf.Exp(-mag) * Data[id].w;
        }
        return weight;
}
//}

== その他


今回リポジトリに３次元の棄却法（円の例で示したような簡単なモンテカルロ法）のサンプルも入っているので比較してみるとよいでしょう。棄却法では棄却の基準値を強めに取るとほとんどサンプリングが上手くできないのに対して、MCMCでは同じようなサンプリング結果をよりスムーズに提示することができます。またMCMCではステップ毎のランダムウォークの幅を小さくすれば、一連の連鎖の中では近しい空間からサンプリングするため、植物や花の群生を簡単に再現することができます。


== 参考文献
 * 久保拓弥（2012）データ解析のための統計モデリング入門――一般化線形モデル・階層ベイズモデル・MCMC (確率と情報の科学) 岩波書店
 * Olle Haggstrom, 野間口 謙太郎 (2017) やさしいMCMC入門: 有限マルコフ連鎖とアルゴリズム 共立出版
