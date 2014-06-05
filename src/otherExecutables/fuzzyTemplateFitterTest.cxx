<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='utf-8'>
<title>
pulsefitter-class/src/otherExecutables/fuzzyTemplateFitterTest.cxx at ddba9462562ffb5e398ddc2a41852c9f6d574fdc - Aaron Fienberg / Pulsefitter Class | 
GitLab
</title>
<link href="/assets/favicon-220424ba6cb497309f8faf8545eb5408.ico" rel="shortcut icon" type="image/vnd.microsoft.icon" />
<link href="/assets/application-92f26cd05f0d0c5a3b1ff1ec228197d0.css" media="all" rel="stylesheet" />
<link href="/assets/print-133dbf6c5049e528c5ed63754a3c2ae7.css" media="print" rel="stylesheet" />
<script src="/assets/application-69f2b8c8cf50852456dfa3b6491a9df7.js"></script>
<meta content="authenticity_token" name="csrf-param" />
<meta content="/6m5e50DDB8Plyu6l6UJ3eD/OG58xA1bVESMe0S2XJ4=" name="csrf-token" />
<script type="text/javascript">
//<![CDATA[
window.gon={};gon.default_issues_tracker="gitlab";gon.api_version="v3";gon.api_token="SibMhryTQ6uTynfxMNXG";gon.gravatar_url="https://secure.gravatar.com/avatar/%{hash}?s=%{size}\u0026d=mm";gon.relative_url_root="";gon.gravatar_enabled=true;
//]]>
</script>
<meta name="viewport" content="width=device-width, initial-scale=1.0">




</head>

<body class='ui_mars project' data-page='projects:blob:show' data-project-id='5'>

<header class='navbar navbar-static-top navbar-gitlab'>
<div class='navbar-inner'>
<div class='container'>
<div class='app_logo'>
<span class='separator'></span>
<a class="home has_bottom_tooltip" href="/" title="Dashboard"><h1>GITLAB</h1>
</a><span class='separator'></span>
</div>
<h1 class='title'><span><a href="/u/aaron">Aaron Fienberg</a> / Pulsefitter Class</span></h1>
<button class='navbar-toggle' data-target='.navbar-collapse' data-toggle='collapse' type='button'>
<span class='sr-only'>Toggle navigation</span>
<i class='icon-reorder'></i>
</button>
<div class='navbar-collapse collapse'>
<ul class='nav navbar-nav'>
<li class='hidden-sm hidden-xs'>
<div class='search'>
<form accept-charset="UTF-8" action="/search" class="navbar-form pull-left" method="get"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /></div>
<input class="search-input" id="search" name="search" placeholder="Search in this project" type="text" />
<input id="group_id" name="group_id" type="hidden" />
<input id="project_id" name="project_id" type="hidden" value="5" />
<input id="search_code" name="search_code" type="hidden" value="true" />
<input id="repository_ref" name="repository_ref" type="hidden" value="ddba9462562ffb5e398ddc2a41852c9f6d574fdc" />

<div class='search-autocomplete-opts hide' data-autocomplete-path='/search/autocomplete' data-autocomplete-project-id='5' data-autocomplete-project-ref='ddba9462562ffb5e398ddc2a41852c9f6d574fdc'></div>
</form>

</div>

</li>
<li class='visible-sm visible-xs'>
<a class="has_bottom_tooltip" data-original-title="Search area" href="/search" title="Search"><i class='icon-search'></i>
</a></li>
<li>
<a class="has_bottom_tooltip" data-original-title="Public area" href="/public" title="Public area"><i class='icon-globe'></i>
</a></li>
<li>
<a class="has_bottom_tooltip" data-original-title="My snippets" href="/s/aaron" title="My snippets"><i class='icon-paste'></i>
</a></li>
<li>
<a class="has_bottom_tooltip" data-original-title="New project" href="/projects/new" title="New project"><i class='icon-plus'></i>
</a></li>
<li>
<a class="has_bottom_tooltip" data-original-title="Profile settings&quot;" href="/profile" title="Profile settings"><i class='icon-user'></i>
</a></li>
<li>
<a class="has_bottom_tooltip" data-method="delete" data-original-title="Logout" href="/users/sign_out" rel="nofollow" title="Logout"><i class='icon-signout'></i>
</a></li>
<li>
<a class="profile-pic" href="/u/aaron" id="profile-pic"><img alt="User activity" src="https://secure.gravatar.com/avatar/60f433fd5afcf27420890cae3ebbcab7?s=26&amp;d=mm" />
</a></li>
</ul>
</div>
</div>
</div>
</header>

<script>
  GitLab.GfmAutoComplete.dataSource = "/aaron/pulsefitter-class/autocomplete_sources?type=NilClass&type_id=ddba9462562ffb5e398ddc2a41852c9f6d574fdc%2Fsrc%2FotherExecutables%2FfuzzyTemplateFitterTest.cxx"
  GitLab.GfmAutoComplete.setup();
</script>

<div class='flash-container'>
</div>


<nav class='main-nav navbar-collapse collapse'>
<div class='container'><ul>
<li class="home"><a href="/aaron/pulsefitter-class" title="Project"><i class='icon-home'></i>
</a></li><li class="active"><a href="/aaron/pulsefitter-class/tree/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">Files</a>
</li><li class=""><a href="/aaron/pulsefitter-class/commits/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">Commits</a>
</li><li class=""><a href="/aaron/pulsefitter-class/network/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">Network</a>
</li><li class=""><a href="/aaron/pulsefitter-class/graphs/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">Graphs</a>
</li><li class=""><a href="/aaron/pulsefitter-class/issues">Issues
<span class='count issue_counter'>0</span>
</a></li><li class=""><a href="/aaron/pulsefitter-class/merge_requests">Merge Requests
<span class='count merge_counter'>0</span>
</a></li><li class=""><a href="/aaron/pulsefitter-class/wikis/home">Wiki</a>
</li><li class=""><a class="stat-tab tab " href="/aaron/pulsefitter-class/edit">Settings
</a></li></ul>
</div>
</nav>
<div class='container'>
<div class='content'><div class='tree-ref-holder'>
<form accept-charset="UTF-8" action="/aaron/pulsefitter-class/refs/switch" class="project-refs-form" method="get"><div style="margin:0;padding:0;display:inline"><input name="utf8" type="hidden" value="&#x2713;" /></div>
<select class="project-refs-select select2 select2-sm" id="ref" name="ref"><optgroup label="Branches"><option value="experimentalFuzzyTemplate">experimentalFuzzyTemplate</option>
<option value="fuzzyTemplateFitter">fuzzyTemplateFitter</option>
<option value="master">master</option></optgroup><optgroup label="Tags"></optgroup><optgroup label="Commit"><option selected="selected" value="ddba9462562ffb5e398ddc2a41852c9f6d574fdc">ddba9462562ffb5e398ddc2a41852c9f6d574fdc</option></optgroup></select>
<input id="destination" name="destination" type="hidden" value="blob" />
<input id="path" name="path" type="hidden" value="src/otherExecutables/fuzzyTemplateFitterTest.cxx" />
</form>


</div>
<div class='tree-holder' id='tree-holder'>
<ul class='breadcrumb'>
<li>
<i class='icon-angle-right'></i>
<a href="/aaron/pulsefitter-class/tree/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">pulsefitter-class
</a></li>
<li>
<a href="/aaron/pulsefitter-class/tree/ddba9462562ffb5e398ddc2a41852c9f6d574fdc/src">src</a>
</li>
<li>
<a href="/aaron/pulsefitter-class/tree/ddba9462562ffb5e398ddc2a41852c9f6d574fdc/src/otherExecutables">otherExecutables</a>
</li>
<li>
<a href="/aaron/pulsefitter-class/blob/ddba9462562ffb5e398ddc2a41852c9f6d574fdc/src/otherExecutables/fuzzyTemplateFitterTest.cxx"><span class='cblue'>
fuzzyTemplateFitterTest.cxx
</span>
</a></li>
</ul>
<ul class='blob-commit-info bs-callout bs-callout-info'>
<li class='commit js-toggle-container'>
<div class='commit-row-title'>
<a class="commit_short_id" href="/aaron/pulsefitter-class/commit/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">ddba94625</a>
&nbsp;
<span class='str-truncated'>
<a class="commit-row-message" href="/aaron/pulsefitter-class/commit/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">made pulseFitFunction a private class definition in pulseFitter</a>
</span>
<a class="pull-right" href="/aaron/pulsefitter-class/tree/ddba9462562ffb5e398ddc2a41852c9f6d574fdc">Browse Code »</a>
<div class='notes_count'>
</div>
</div>
<div class='commit-row-info'>
<a class="commit-author-link has_tooltip" data-original-title="fienberg@uw.edu" href="/u/aaron"><img alt="" class="avatar s16" src="https://secure.gravatar.com/avatar/60f433fd5afcf27420890cae3ebbcab7?s=16&amp;d=mm" width="16" /> <span class="commit-author-name">Aaron Fienberg</span></a>
<div class='committed_ago'>
<time class='time_ago' data-placement='top' data-toggle='tooltip' datetime='2014-05-31T00:38:27Z' title='May 30, 2014 5:38pm'>2014-05-30 17:38:27 -0700</time>
<script>$('.time_ago').timeago().tooltip()</script>
 &nbsp;
</div>
</div>
</li>

</ul>
<div class='tree-content-holder' id='tree-content-holder'>
<div class='file-holder'>
<div class='file-title'>
<i class='icon-file'></i>
<span class='file_name'>
fuzzyTemplateFitterTest.cxx
<small>1.6 KB</small>
</span>
<span class='options'><div class='btn-group tree-btn-group'>
<span class='btn btn-small disabled'>edit</span>
<a class="btn btn-small" href="/aaron/pulsefitter-class/raw/ddba9462562ffb5e398ddc2a41852c9f6d574fdc/src/otherExecutables/fuzzyTemplateFitterTest.cxx" target="_blank">raw</a>
<a class="btn btn-small" href="/aaron/pulsefitter-class/blame/ddba9462562ffb5e398ddc2a41852c9f6d574fdc/src/otherExecutables/fuzzyTemplateFitterTest.cxx">blame</a>
<a class="btn btn-small" href="/aaron/pulsefitter-class/commits/ddba9462562ffb5e398ddc2a41852c9f6d574fdc/src/otherExecutables/fuzzyTemplateFitterTest.cxx">history</a>
</div>
</span>
</div>
<div class='file-content code'>
<div class='highlighted-data white'>
<div class='line-numbers'>
<a href="#L1" id="L1" rel="#L1"><i class='icon-link'></i>
1
</a><a href="#L2" id="L2" rel="#L2"><i class='icon-link'></i>
2
</a><a href="#L3" id="L3" rel="#L3"><i class='icon-link'></i>
3
</a><a href="#L4" id="L4" rel="#L4"><i class='icon-link'></i>
4
</a><a href="#L5" id="L5" rel="#L5"><i class='icon-link'></i>
5
</a><a href="#L6" id="L6" rel="#L6"><i class='icon-link'></i>
6
</a><a href="#L7" id="L7" rel="#L7"><i class='icon-link'></i>
7
</a><a href="#L8" id="L8" rel="#L8"><i class='icon-link'></i>
8
</a><a href="#L9" id="L9" rel="#L9"><i class='icon-link'></i>
9
</a><a href="#L10" id="L10" rel="#L10"><i class='icon-link'></i>
10
</a><a href="#L11" id="L11" rel="#L11"><i class='icon-link'></i>
11
</a><a href="#L12" id="L12" rel="#L12"><i class='icon-link'></i>
12
</a><a href="#L13" id="L13" rel="#L13"><i class='icon-link'></i>
13
</a><a href="#L14" id="L14" rel="#L14"><i class='icon-link'></i>
14
</a><a href="#L15" id="L15" rel="#L15"><i class='icon-link'></i>
15
</a><a href="#L16" id="L16" rel="#L16"><i class='icon-link'></i>
16
</a><a href="#L17" id="L17" rel="#L17"><i class='icon-link'></i>
17
</a><a href="#L18" id="L18" rel="#L18"><i class='icon-link'></i>
18
</a><a href="#L19" id="L19" rel="#L19"><i class='icon-link'></i>
19
</a><a href="#L20" id="L20" rel="#L20"><i class='icon-link'></i>
20
</a><a href="#L21" id="L21" rel="#L21"><i class='icon-link'></i>
21
</a><a href="#L22" id="L22" rel="#L22"><i class='icon-link'></i>
22
</a><a href="#L23" id="L23" rel="#L23"><i class='icon-link'></i>
23
</a><a href="#L24" id="L24" rel="#L24"><i class='icon-link'></i>
24
</a><a href="#L25" id="L25" rel="#L25"><i class='icon-link'></i>
25
</a><a href="#L26" id="L26" rel="#L26"><i class='icon-link'></i>
26
</a><a href="#L27" id="L27" rel="#L27"><i class='icon-link'></i>
27
</a><a href="#L28" id="L28" rel="#L28"><i class='icon-link'></i>
28
</a><a href="#L29" id="L29" rel="#L29"><i class='icon-link'></i>
29
</a><a href="#L30" id="L30" rel="#L30"><i class='icon-link'></i>
30
</a><a href="#L31" id="L31" rel="#L31"><i class='icon-link'></i>
31
</a><a href="#L32" id="L32" rel="#L32"><i class='icon-link'></i>
32
</a><a href="#L33" id="L33" rel="#L33"><i class='icon-link'></i>
33
</a><a href="#L34" id="L34" rel="#L34"><i class='icon-link'></i>
34
</a><a href="#L35" id="L35" rel="#L35"><i class='icon-link'></i>
35
</a><a href="#L36" id="L36" rel="#L36"><i class='icon-link'></i>
36
</a><a href="#L37" id="L37" rel="#L37"><i class='icon-link'></i>
37
</a><a href="#L38" id="L38" rel="#L38"><i class='icon-link'></i>
38
</a><a href="#L39" id="L39" rel="#L39"><i class='icon-link'></i>
39
</a><a href="#L40" id="L40" rel="#L40"><i class='icon-link'></i>
40
</a><a href="#L41" id="L41" rel="#L41"><i class='icon-link'></i>
41
</a><a href="#L42" id="L42" rel="#L42"><i class='icon-link'></i>
42
</a><a href="#L43" id="L43" rel="#L43"><i class='icon-link'></i>
43
</a><a href="#L44" id="L44" rel="#L44"><i class='icon-link'></i>
44
</a><a href="#L45" id="L45" rel="#L45"><i class='icon-link'></i>
45
</a><a href="#L46" id="L46" rel="#L46"><i class='icon-link'></i>
46
</a><a href="#L47" id="L47" rel="#L47"><i class='icon-link'></i>
47
</a><a href="#L48" id="L48" rel="#L48"><i class='icon-link'></i>
48
</a><a href="#L49" id="L49" rel="#L49"><i class='icon-link'></i>
49
</a><a href="#L50" id="L50" rel="#L50"><i class='icon-link'></i>
50
</a><a href="#L51" id="L51" rel="#L51"><i class='icon-link'></i>
51
</a><a href="#L52" id="L52" rel="#L52"><i class='icon-link'></i>
52
</a><a href="#L53" id="L53" rel="#L53"><i class='icon-link'></i>
53
</a><a href="#L54" id="L54" rel="#L54"><i class='icon-link'></i>
54
</a><a href="#L55" id="L55" rel="#L55"><i class='icon-link'></i>
55
</a><a href="#L56" id="L56" rel="#L56"><i class='icon-link'></i>
56
</a><a href="#L57" id="L57" rel="#L57"><i class='icon-link'></i>
57
</a><a href="#L58" id="L58" rel="#L58"><i class='icon-link'></i>
58
</a><a href="#L59" id="L59" rel="#L59"><i class='icon-link'></i>
59
</a><a href="#L60" id="L60" rel="#L60"><i class='icon-link'></i>
60
</a><a href="#L61" id="L61" rel="#L61"><i class='icon-link'></i>
61
</a><a href="#L62" id="L62" rel="#L62"><i class='icon-link'></i>
62
</a><a href="#L63" id="L63" rel="#L63"><i class='icon-link'></i>
63
</a><a href="#L64" id="L64" rel="#L64"><i class='icon-link'></i>
64
</a><a href="#L65" id="L65" rel="#L65"><i class='icon-link'></i>
65
</a><a href="#L66" id="L66" rel="#L66"><i class='icon-link'></i>
66
</a><a href="#L67" id="L67" rel="#L67"><i class='icon-link'></i>
67
</a><a href="#L68" id="L68" rel="#L68"><i class='icon-link'></i>
68
</a><a href="#L69" id="L69" rel="#L69"><i class='icon-link'></i>
69
</a><a href="#L70" id="L70" rel="#L70"><i class='icon-link'></i>
70
</a><a href="#L71" id="L71" rel="#L71"><i class='icon-link'></i>
71
</a><a href="#L72" id="L72" rel="#L72"><i class='icon-link'></i>
72
</a><a href="#L73" id="L73" rel="#L73"><i class='icon-link'></i>
73
</a><a href="#L74" id="L74" rel="#L74"><i class='icon-link'></i>
74
</a><a href="#L75" id="L75" rel="#L75"><i class='icon-link'></i>
75
</a><a href="#L76" id="L76" rel="#L76"><i class='icon-link'></i>
76
</a><a href="#L77" id="L77" rel="#L77"><i class='icon-link'></i>
77
</a><a href="#L78" id="L78" rel="#L78"><i class='icon-link'></i>
78
</a><a href="#L79" id="L79" rel="#L79"><i class='icon-link'></i>
79
</a><a href="#L80" id="L80" rel="#L80"><i class='icon-link'></i>
80
</a></div>
<div class='highlight'>
<pre><code>#include &quot;pulseFitter.hh&quot;
#include &quot;TTree.h&quot;
#include &lt;iostream&gt;
#include &quot;TApplication.h&quot;
#include &quot;TH1.h&quot;
#include &quot;TCanvas.h&quot;
#include &quot;TFile.h&quot;
#include &quot;TSystem.h&quot;

using namespace std;
const int DEFAULTSTRUCKCHANNEL = 6;

struct s_sis{
  unsigned long long timestamp[8];
  unsigned short trace[8][0x400];
  bool is_bad_event;
};

int main(int argc, char* argv[]){
  if(argc&lt;2){
    cout &lt;&lt; &quot;need to input datafile.&quot; &lt;&lt; endl;
    return -1;
  }
  
  int struckChannel = DEFAULTSTRUCKCHANNEL;
  if(argc == 3){
    struckChannel = atoi(argv[2]);
  }


  new TApplication(&quot;app&quot;, 0, nullptr);
  
  typedef struct {
    double energy;
    double chi2;
    double sum;
    double baseline;
  } fitResults;
  
  gSystem-&gt;Load(&quot;libTree&quot;);
  TFile infile(argv[1]);
  TTree* t = (TTree*) infile.Get(&quot;t&quot;);
  struct s_sis s;
  t-&gt;SetBranchAddress(&quot;sis&quot;, &amp;s);

  fitResults fr;
  pulseFitter pf((char*)&quot;configs/fuzzyConfig.json&quot;,true);

  TFile* outf = new TFile(&quot;outfile.root&quot;,&quot;recreate&quot;);
  TTree* outTree = new TTree(&quot;t&quot;,&quot;t&quot;);
  outTree-&gt;Branch(&quot;fitResults&quot;,&amp;fr,&quot;energy/D:chi2/D:sum/D:baseline/D&quot;);
  
  

  for(int i = 0; i &lt;t-&gt;GetEntries(); ++i){
    t-&gt;GetEntry(i);
    pf.fitSingle(s.trace[struckChannel]);
    fr.energy = pf.getScale();
    fr.baseline = pf.getBaseline();
    fr.chi2 = pf.getChi2();
    fr.sum = -1*pf.getSum(s.trace[struckChannel],150,100);
    if(i%1000==0){
      cout &lt;&lt; i &lt;&lt; &quot; done&quot; &lt;&lt; endl;
    }
    if(pf.wasValidFit()){
      outTree-&gt;Fill();
    }
  }
  cout &lt;&lt; outTree-&gt;GetEntries() &lt;&lt; &quot; successful fits&quot; &lt;&lt; endl;
  outTree-&gt;Write();
  outf-&gt;Write();
  
  delete outTree;
  delete outf;
  delete t;
}</code></pre>
</div>
</div>

</div>

</div>
</div>

</div>
</div>
</div>
</body>
</html>
