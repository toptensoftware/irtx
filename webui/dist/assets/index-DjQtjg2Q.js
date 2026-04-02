var oi=Object.defineProperty;var St=s=>{throw TypeError(s)};var ai=(s,e,t)=>e in s?oi(s,e,{enumerable:!0,configurable:!0,writable:!0,value:t}):s[e]=t;var M=(s,e,t)=>ai(s,typeof e!="symbol"?e+"":e,t),ot=(s,e,t)=>e.has(s)||St("Cannot "+t);var o=(s,e,t)=>(ot(s,e,"read from private field"),t?t.call(s):e.get(s)),w=(s,e,t)=>e.has(s)?St("Cannot add the same private member more than once"):e instanceof WeakSet?e.add(s):e.set(s,t),g=(s,e,t,i)=>(ot(s,e,"write to private field"),i?i.call(s,t):e.set(s,t),t),C=(s,e,t)=>(ot(s,e,"access private method"),t);var _e=(s,e,t,i)=>({set _(n){g(s,e,n,t)},get _(){return o(s,e,i)}});(function(){const e=document.createElement("link").relList;if(e&&e.supports&&e.supports("modulepreload"))return;for(const n of document.querySelectorAll('link[rel="modulepreload"]'))i(n);new MutationObserver(n=>{for(const r of n)if(r.type==="childList")for(const l of r.addedNodes)l.tagName==="LINK"&&l.rel==="modulepreload"&&i(l)}).observe(document,{childList:!0,subtree:!0});function t(n){const r={};return n.integrity&&(r.integrity=n.integrity),n.referrerPolicy&&(r.referrerPolicy=n.referrerPolicy),n.crossOrigin==="use-credentials"?r.credentials="include":n.crossOrigin==="anonymous"?r.credentials="omit":r.credentials="same-origin",r}function i(n){if(n.ep)return;n.ep=!0;const r=t(n);fetch(n.href,r)}})();function at(s){return s.replace(/[A-Z]/g,e=>`-${e.toLowerCase()}`)}function ct(s){return s instanceof Function&&!!s.prototype&&!!s.prototype.constructor}let li=/^[a-zA-Z$][a-zA-Z0-9_$]*$/;function me(s){return s.match(li)?`.${s}`:`[${JSON.stringify(s)}]`}function Mt(s,e){s.loading?s.addEventListener("loaded",e,{once:!0}):e()}function di(s){return new Promise(e=>Mt(s,e))}var oe;class ci extends EventTarget{constructor(){super();w(this,oe,0);this.browser=!1,this.ssr=!1}enterLoading(){_e(this,oe)._++,o(this,oe)==1&&this.dispatchEvent(new Event("loading"))}leaveLoading(){_e(this,oe)._--,o(this,oe)==0&&this.dispatchEvent(new Event("loaded"))}get loading(){return o(this,oe)!=0}async load(t){this.enterLoading();try{return await t()}finally{this.leaveLoading()}}untilLoaded(){return di(this)}}oe=new WeakMap;let Ct;Object.defineProperty(globalThis,"coenv",{get(){return Ct()}});function hi(s){Ct=s}class I{constructor(e){M(this,"html");this.html=e}static areEqual(e,t){return e instanceof I&&t instanceof I&&e.html==t.html}}function Dt(s){return s instanceof Function?(...e)=>new I(s(...e)):new I(s)}class ui{constructor(e){this.value=e}}class fi{static declare(e){coenv.declareStyle(e)}}function Ue(s,e){let t="";for(let i=0;i<s.length-1;i++)t+=s[i],t+=e[i];t+=s[s.length-1],fi.declare(t)}let ke=[],lt=!1;function tt(s,e){s&&(e=e??0,e!=0&&(lt=!0),ke.push({callback:s,order:e}),ke.length==1&&coenv.window.requestAnimationFrame(function(){let t=ke;lt&&(t.sort((i,n)=>n.order-i.order),lt=!1),ke=[];for(let i=t.length-1;i>=0;i--)t[i].callback()}))}function Et(s){ke.length==0?s():tt(s,Number.MAX_SAFE_INTEGER)}function pi(){return ke.length!=0}function Ge(){let s=[],e="",t=!0;function i(...c){for(let m=0;m<c.length;m++){let f=c[m];f.lines?s.push(...f.lines.map(u=>e+u)):Array.isArray(f)?s.push(...f.filter(u=>u!=null).map(u=>e+u)):t?s.push(...f.split(`
`).map(u=>e+u)):s.push(e+f)}}function n(){e+="  "}function r(){e=e.substring(2)}function l(){return s.join(`
`)+`
`}function d(c){i("{"),n(),c(this),r(),i("}")}return{append:i,indent:n,unindent:r,braced:d,toString:l,lines:s,enableSplit(c){t=c},get isEmpty(){return s.length==0}}}class wt{constructor(){this.code=Ge(),this.code.closure=this,this.functions=[],this.locals=[],this.prologs=[],this.epilogs=[]}get isEmpty(){return this.code.isEmpty&&this.locals.length==0&&this.functions.every(e=>e.code.isEmpty)&&this.prologs.every(e=>e.isEmpty)&&this.epilogs.every(e=>e.isEmpty)}addProlog(){let e=Ge();return this.prologs.push(e),e}addEpilog(){let e=Ge();return this.epilogs.push(e),e}addLocal(e,t){this.locals.push({name:e,init:t})}addFunction(e,t){t||(t=[]);let i={name:e,args:t,code:new wt};return this.functions.push(i),i.code}getFunction(e){var t;return(t=this.functions.find(i=>i.name==e))==null?void 0:t.code}toString(){let e=Ge();return this.appendTo(e),e.toString()}appendTo(e){this.locals.length>0&&e.append(`let ${this.locals.map(t=>t.init?`${t.name} = ${t.init}`:t.name).join(", ")};`);for(let t of this.prologs)e.append(t);e.append(this.code);for(let t of this.functions)e.append(`function ${t.name}(${t.args.join(", ")})`),e.append("{"),e.indent(),t.code.appendTo(e),e.unindent(),e.append("}");for(let t of this.epilogs)e.append(t)}}function Ze(s){return s==null?"":(""+s).replace(/["'&<>]/g,function(e){switch(e){case'"':return"&quot;";case"&":return"&amp;";case"'":return"&#39;";case"<":return"&lt;";case">":return"&gt;"}})}function mi(s){let e,t,i=s.event??"input",n,r,l,d,c,m=s.parse??(h=>h),f=s.format??(h=>h);if(typeof s=="string"&&(s={prop:s}),s.get&&s.set)n=()=>s.get(t.model,t),r=h=>s.set(t.model,h,t);else if(typeof s.prop=="string"){let D=function(){let R=h();for(let E=0;E<$.length;E++)R=R[$[E]];return R};var z=D;let h;s.target instanceof Function?h=()=>s.target(t.model,t):s.target?h=()=>s.target:h=()=>t.model;let $=s.prop.split("."),L=$.pop();n=()=>D()[L],r=R=>D()[L]=R}else throw new Error("Invalid input binding");function u(h){var L;let $=d(e[c]);$!==void 0&&($=m($)),$!==void 0&&r($),(L=s.on_change)==null||L.call(s,t.model,h)}function _(){let h=f(n());h!==void 0&&(h=l(h)),h!==void 0&&(e[c]=h)}function v(h,$){if(e=h,t=$,c="value",l=L=>L,d=L=>L,e.tagName=="INPUT"){let L=e.getAttribute("type").toLowerCase();L=="checkbox"?(c="checked",d=D=>!!D):L=="radio"&&(c="checked",d=D=>D?e.getAttribute("value"):void 0,l=D=>e.getAttribute("value")==D?!0:void 0)}else e.tagName=="SELECT"&&e.hasAttribute("multiple")&&(c="selectedOptions",d=L=>Array.from(L).map(D=>D.value),l=L=>{Array.from(e.options).forEach(D=>{D.selected=L.indexOf(D.value)>=0})});e.addEventListener(i,u)}function P(){e.removeEventListener(i,u),e=null}return{create:v,update:_,destroy:P}}class Me{static rawText(e){return e instanceof I?e.html:Ze(e)}static renderToString(e){let t="";return e({write:function(i){t+=i}}),t}static renderComponentToString(e){let t="";return e.render({write:function(i){t+=i}}),t}static rawStyle(e){let t;return e instanceof I?t=e.html:t=Ze(e),t=t.trim(),t.endsWith(";")||(t+=";"),t}static rawNamedStyle(e,t){if(!t)return"";let i;return t instanceof I?i=t.html:i=Ze(t),i=i.trim(),i+=";",`${e}:${i}`}static createTextNode(e){if(e instanceof I){let t=document.createElement("SPAN");return t.innerHTML=e.html,t}else return document.createTextNode(e)}static setElementAttribute(e,t,i){i===void 0?e.removeAttribute(t):e.setAttribute(t,i)}static setElementText(e,t){t instanceof I?e.innerHTML=t.html:e.textContent=t}static setNodeText(e,t){if(t instanceof I){if(e.nodeType==1)return e.innerHTML=t.html,e;let i=document.createElement("SPAN");return i.innerHTML=t.html,e.replaceWith(i),i}else{if(e.nodeType==3)return e.nodeValue=t,e;let i=document.createTextNode(t);return e.replaceWith(i),i}}static setNodeClass(e,t,i){i?e.classList.add(t):e.classList.remove(t)}static setNodeStyle(e,t,i){i==null?e.style.removeProperty(t):e.style[t]=i}static boolClassMgr(e,t,i,n){let r=null,l;return function(){let c=n(e.model,e);c!=l&&(l=c,n.withTransition&&t.isConnected?(r==null||r.finish(),r=n.withTransition(e),c?(r.enterNodes([t]),r.onWillEnter(()=>t.classList.add(i))):(r.leaveNodes([t]),r.onDidLeave(()=>t.classList.remove(i))),r.start()):Me.setNodeClass(t,i,c))}}static setNodeDisplay(e,t,i){if(t===!0){i===null?e.style.removeProperty("display"):i!==void 0&&e.style.display!=i&&(e.style.display=i);return}else if(t===!1||t===null||t===void 0){let n=e.style.display;return e.style.display!="none"&&(e.style.display="none"),n??null}else if(typeof t=="string"){let n=e.style.display;return e.style.display!=t&&(e.style.display=t),n??null}}static displayMgr(e,t,i){let n=null,r,l;return function(){let c=i(e.model,e);if(c!=r){if(r=c,coenv.browser&&i.withTransition&&t.isConnected){n==null||n.finish();let m=window.getComputedStyle(t).getPropertyValue("display"),f;if(c===!0?f=l:c===!1||c===null||c===void 0?f="none":f=c,m=="none"!=(f=="none")){n=i.withTransition(e),f!="none"?(n.enterNodes([t]),n.onWillEnter(()=>l=Me.setNodeDisplay(t,c,l))):(n.leaveNodes([t]),n.onDidLeave(()=>l=Me.setNodeDisplay(t,c,l))),n.start();return}}l=Me.setNodeDisplay(t,c,l)}}}static replaceMany(e,t){var i;if((i=e==null?void 0:e[0])!=null&&i.parentNode){e[0].replaceWith(...t);for(let n=1;n<e.length;n++)e[n].remove()}}static addEventListener(e,t,i,n){function r(l){let d=e();return n(d.model,l,d)}return t.addEventListener(i,r),function(){t.removeEventListener(i,r)}}static input(){return mi(...arguments)}}class Oe{static register(e){this.plugins.push(e)}static transform(e){for(let t of this.plugins)t.transform&&(e=t.transform(e));return e}static transformGroup(e){for(let t of this.plugins)t.transformGroup&&(e=t.transformGroup(e));return e}}M(Oe,"plugins",[]);function gi(s){let e=s.match(/\s*([^ \t.#]*)/);if(!e)return{};s=s.substring(e[0].length);let t=[],i={type:e[1]};for(let n of s.matchAll(/\s*([.#]?)([^ \t=.#]+)(?:\s*=\s*(\'[^']+\'|\S+))?/g)){if(!n[3]){if(n[1]=="."){t.push(n[2]);continue}else if(n[1]=="#"){i.id=n[2];continue}}let r=n[2];r=="type"&&(r="attr_type");let l=n[3]??n[2];(l.startsWith("'")&&l.endsWith("'")||l.startsWith('"')&&l.endsWith('"'))&&(l=l.substring(1,l.length-1)),i[r]=l}return t.length>0&&(i.class=t.join(" ")),i}class ne{constructor(e){if(e.$node&&(e=e.$node),Array.isArray(e)?e={$:e}:typeof e=="object"&&!(e instanceof I)&&(e=Object.assign({},e)),typeof e.type=="string"&&e.type[0]!="#"&&Object.assign(e,gi(e.type)),e=Oe.transform(e),ct(e)&&(e={type:e}),this.template=e,ct(e.type))e.type.integrate?this.kind="integrated":this.kind="component";else if(typeof e=="string")this.kind="text";else if(e instanceof I){if(this.kind="html",this.html=e.html,coenv.document){let t=coenv.document.createElement("div");t.innerHTML=e.html,this.nodes=[...t.childNodes],this.nodes.forEach(i=>i.remove())}}else e instanceof Function?this.kind="dynamic_text":e.type==="#comment"?this.kind="comment":e.type===void 0?this.kind="fragment":this.kind="element";if(this.kind==="integrated"&&(e.$&&!e.content&&(e.content=e.$,delete e.$),this.integrated=this.template.type.integrate(this.template)),this.kind=="element"&&e.$&&!e.text&&(typeof e.$=="string"||e.$ instanceof I)&&(e.text=e.$,delete e.$),this.kind=="element"||this.kind=="fragment")e.$&&!e.childNodes&&(e.childNodes=e.$,delete e.$),e.childNodes?(Array.isArray(e.childNodes)?e.childNodes=e.childNodes.flat():e.childNodes=[e.childNodes],e.childNodes=e.childNodes.map(t=>t.$node??t),this.childNodes=Oe.transformGroup(e.childNodes).map(t=>new ne(t))):this.childNodes=[];else if(this.isComponent)e.$&&!e.content&&(e.content=e.$,delete e.$);else if(e.childNodes)throw new Error("childNodes only supported on element and fragment nodes")}get isSingleRoot(){return this.isFragment?this.childNodes.length==1&&this.childNodes[0].isSingleRoot:this.isComponent?this.template.type.isSingleRoot:this.isIntegrated?this.integrated.isSingleRoot:this.kind=="html"?this.nodes.length==1:!0}get isComponent(){return this.kind==="component"}get isFragment(){return this.kind==="fragment"}get isIntegrated(){return this.kind==="integrated"}*enumLocalNodes(){if(this.isFragment||(yield this),this.childNodes)for(let e=0;e<this.childNodes.length;e++)yield*this.childNodes[e].enumLocalNodes()}spreadChildDomNodes(){return Array.from(e(this)).filter(t=>t.length>0).join(", ");function*e(t){for(let i=0;i<t.childNodes.length;i++)yield t.childNodes[i].spreadDomNodes()}}spreadDomNodes(){return Array.from(this.enumAllNodes()).join(", ")}*enumAllNodes(){switch(this.kind){case"fragment":for(let e=0;e<this.childNodes.length;e++)yield*this.childNodes[e].enumAllNodes();break;case"component":case"integrated":this.isSingleRoot?yield`${this.name}.rootNode`:yield`...${this.name}.rootNodes`;break;case"html":this.nodes.length>0&&(this.nodes.length>1?yield`...${this.name}`:yield`${this.name}`);break;default:yield this.name}}}let Rt={enterNodes:function(){},leaveNodes:function(){},onWillEnter:function(s){s()},onDidLeave:function(s){s()},start:function(){},finish:function(){}};var W,H,K,O,Ce,De,qe,We,ie,Ae,ee,Pt,ae;const Qe=class Qe{constructor(e){w(this,ie);w(this,W);w(this,H);w(this,K);w(this,O);w(this,Ce);w(this,De);w(this,qe);w(this,We);w(this,ee,!0);w(this,ae);var t,i;g(this,W,e.context),g(this,qe,e.nodes[1]),g(this,Ce,(t=coenv.document)==null?void 0:t.createTextNode("")),g(this,De,(i=coenv.document)==null?void 0:i.createTextNode("")),g(this,ee,e.data.ownsContent??!0),e.nodes[0]?this.content=e.nodes[0]():this.content=e.data.content}static integrate(e){let t=null;e.content&&typeof e.content=="object"&&(t=e.content,delete e.content);let i={isSingleRoot:!1,data:{ownsContent:e.ownsContent??!0,content:e.content},nodes:[t?new ne(t):null,e.placeholder?new ne(e.placeholder):null]};return delete e.content,delete e.placeholder,delete e.ownsContent,i}static transform(e){return e instanceof Function&&!ct(e)?{type:Qe,content:e}:(e.type=="embed-slot"&&(e.type=Qe),e)}get rootNodes(){return[o(this,Ce),...o(this,ie,Ae),o(this,De)]}get isSingleRoot(){return!1}get ownsContent(){return o(this,ee)}set ownsContent(e){g(this,ee,e)}get content(){return o(this,H)}set content(e){g(this,H,e),o(this,H)instanceof Function?this.replaceContent(o(this,H).call(o(this,W).model,o(this,W).model,o(this,W))):this.replaceContent(o(this,H))}update(){var e,t;o(this,H)instanceof Function&&this.replaceContent(o(this,H).call(o(this,W).model,o(this,W).model,o(this,W))),o(this,K)||(t=(e=o(this,O))==null?void 0:e.update)==null||t.call(e)}bind(){var e,t;o(this,K)||(t=(e=o(this,O))==null?void 0:e.bind)==null||t.call(e)}unbind(){var e,t;o(this,K)||(t=(e=o(this,O))==null?void 0:e.unbind)==null||t.call(e)}get isAttached(){}setMounted(e){var t,i;g(this,ae,e),(i=(t=o(this,O))==null?void 0:t.setMounted)==null||i.call(t,e)}replaceContent(e){var r,l,d,c,m,f,u,_;if((o(this,K)===e||I.areEqual(o(this,K),e))&&(e||o(this,O)))return;let t=o(this,O),i=[...o(this,ie,Ae)];g(this,K,e);let n;if(!e)n=((r=o(this,qe))==null?void 0:r.call(this,o(this,W)))??null;else if(e.rootNodes)n=e;else if(e instanceof I){let v=coenv.document.createElement("span");v.innerHTML=e.html,n=[...v.childNodes],n.forEach(P=>P.remove())}else if(typeof e=="string")n=[coenv.document.createTextNode(e)];else if(Array.isArray(e))n=e;else if(e.nodeType!==void 0)n=[e];else if(e.render)n=e;else throw new Error("Embed slot requires component, array of HTML nodes or a single HTML node");if(g(this,O,n),o(this,ie,Pt)){(l=o(this,We))==null||l.finish();let v;o(this,ae)&&(v=(c=(d=o(this,H))==null?void 0:d.withTransition)==null?void 0:c.call(d,o(this,W))),v||(v=Rt),g(this,We,v),v.enterNodes(o(this,ie,Ae)),v.leaveNodes(i),v.onWillEnter(()=>{var P,z;o(this,De).before(...o(this,ie,Ae)),o(this,ae)&&((z=(P=o(this,O))==null?void 0:P.setMounted)==null||z.call(P,!0))}),v.onDidLeave(()=>{var P,z;i.forEach(h=>h.remove()),o(this,ae)&&((P=t==null?void 0:t.setMounted)==null||P.call(t,!1)),o(this,ee)&&((z=t==null?void 0:t.destroy)==null||z.call(t))}),v.start()}else o(this,ae)&&((m=t==null?void 0:t.setMounted)==null||m.call(t,!1),(u=(f=o(this,O))==null?void 0:f.setMounted)==null||u.call(f,!0)),o(this,ee)&&((_=t==null?void 0:t.destroy)==null||_.call(t))}destroy(){var e,t;o(this,ee)&&((t=(e=o(this,O))==null?void 0:e.destroy)==null||t.call(e))}};W=new WeakMap,H=new WeakMap,K=new WeakMap,O=new WeakMap,Ce=new WeakMap,De=new WeakMap,qe=new WeakMap,We=new WeakMap,ie=new WeakSet,Ae=function(){var e;return((e=o(this,O))==null?void 0:e.rootNodes)??o(this,O)??[]},ee=new WeakMap,Pt=function(){var e;return((e=o(this,Ce))==null?void 0:e.parentNode)!=null},ae=new WeakMap;let ht=Qe;Oe.register(ht);function yi(s,e){let t=Math.min(s.length,e.length),i=Math.max(s.length,e.length),n=0;for(;n<t&&s[n]==e[n];)n++;if(n==i)return[];if(n==s.length)return[{op:"insert",index:s.length,count:e.length-s.length}];let r=0;for(;r<t-n&&s[s.length-r-1]==e[e.length-r-1];)r++;if(r==s.length)return[{op:"insert",index:0,count:e.length-s.length}];if(n+r==s.length)return[{op:"insert",index:n,count:e.length-s.length}];if(n+r==e.length)return[{op:"delete",index:n,count:s.length-e.length}];let l=s.length-r,d=e.length-r,c=z(e,n,d),m=null,f=[],u=n,_=n;for(;u<d;){for(;u<d&&s[_]==e[u];)c.delete(e[u],u),u++,_++;let h=u,$=_;for(;_<l&&!c.has(s[_]);)_++;if(_>$){f.push({op:"delete",index:h,count:_-$});continue}for(m||(m=z(s,u,l));u<d&&!m.has(e[u]);)c.delete(e[u],u),u++;if(u>h){f.push({op:"insert",index:h,count:u-h});continue}break}if(u==d)return f;let v=0,P=new _t;for(;_<l;){let h=_;for(;_<l&&!c.has(s[_]);)_++;if(_>h){f.push({op:"delete",index:u,count:_-h});continue}for(;_<l&&c.consume(s[_])!==void 0;)P.add(s[_],v++),_++;_>h&&f.push({op:"store",index:u,count:_-h})}for(;u<d;){let h=u;for(;u<d&&!P.has(e[u]);)u++;if(u>h){f.push({op:"insert",index:h,count:u-h});continue}let $={op:"restore",index:u,count:0};for(f.push($);u<d;){let L=P.consume(e[u]);if(L===void 0)break;$.count==0?($.storeIndex=L,$.count=1):$.storeIndex+$.count==L?$.count++:($={op:"restore",index:u,storeIndex:L,count:1},f.push($)),u++}}return f;function z(h,$,L){let D=new _t;for(let R=$;R<L;R++)D.add(h[R],R);return D}}var le;class _t{constructor(){w(this,le,new Map)}add(e,t){let i=o(this,le).get(e);i?i.push(t):o(this,le).set(e,[t])}delete(e,t){let i=o(this,le).get(e);if(i){let n=i.indexOf(t);if(n>=0){i.splice(n,1);return}}throw new Error("key/value pair not found")}consume(e){let t=o(this,le).get(e);if(!(!t||t.length==0))return t.shift()}has(e){return o(this,le).has(e)}}le=new WeakMap;var Re,Z,J,b,Ft,ft,$e,we,Je,be,ye,It,pt,Bt,mt,Ot,gt,Ut,yt,Le;const Ke=class Ke{constructor(e){w(this,b);w(this,Re);w(this,Z);w(this,J,!1);w(this,$e);w(this,we);w(this,Je);w(this,be);var t,i;this.itemConstructor=e.data.itemConstructor,this.outer=e.context,this.items=e.data.template.items,this.condition=e.data.template.condition,this.itemKey=e.data.template.itemKey,this.emptyConstructor=e.nodes.length?e.nodes[0]:null,this.itemDoms=[],g(this,Re,(t=coenv.document)==null?void 0:t.createComment(" enter foreach block ")),g(this,Z,(i=coenv.document)==null?void 0:i.createComment(" leave foreach block ")),this.itemConstructor.isSingleRoot?(g(this,$e,C(this,b,Ot)),g(this,Je,C(this,b,Ut)),g(this,we,C(this,b,gt)),g(this,be,C(this,b,yt))):(g(this,$e,C(this,b,It)),g(this,Je,C(this,b,Bt)),g(this,we,C(this,b,pt)),g(this,be,C(this,b,mt)))}static integrate(e){let t=e.template,i={template:{items:e.items,condition:e.condition,itemKey:e.itemKey}},n;return e.empty&&(n=[new ne(e.empty)]),delete e.template,delete e.items,delete e.condition,delete e.itemKey,delete e.empty,{isSingleRoot:!1,data:i,nodes:n,compile:r=>{i.itemConstructor=r.compileTemplate(t,r)}}}static transform(e){if(e.foreach===void 0)return e;let t;return e.foreach instanceof Function||Array.isArray(e.foreach)?(t={type:Ke,template:e,items:e.foreach},delete e.foreach):(t=Object.assign({},e.foreach,{type:Ke,template:e}),delete e.foreach),t}get rootNodes(){let e=this.emptyDom?this.emptyDom.rootNodes:[];if(this.itemConstructor.isSingleRoot)return[o(this,Re),...this.itemDoms.map(t=>t.rootNode),...e,o(this,Z)];{let t=[o(this,Re)];for(let i=0;i<this.itemDoms.length;i++)t.push(...this.itemDoms[i].rootNodes);return t.push(...e),t.push(o(this,Z)),t}}setMounted(e){g(this,J,e),ge(this.itemDoms,e)}update(){let e;this.items instanceof Function?e=this.items.call(this.outer.model,this.outer.model,this.outer):e=this.items,e=e??[];let t={outer:this.outer},i=null;if(this.condition&&(e=e.filter(n=>(t.model=n,this.condition.call(n,n,t)))),this.itemKey&&(i=e.map(n=>(t.model=n,this.itemKey.call(n,n,t)))),!this.itemsLoaded){this.itemsLoaded=!0,o(this,$e).call(this,e,i,0,0,e.length),C(this,b,ft).call(this);return}C(this,b,Ft).call(this,0,this.itemDoms.length,e,i)}bind(){var e,t;(t=(e=this.emptyDom)==null?void 0:e.bind)==null||t.call(e)}unbind(){var e,t;(t=(e=this.emptyDom)==null?void 0:e.unbind)==null||t.call(e)}destroy(){Xe(this.itemDoms),this.itemDoms=null}};Re=new WeakMap,Z=new WeakMap,J=new WeakMap,b=new WeakSet,Ft=function(e,t,i,n){let r=e+t,l;e==0&&t==this.itemDoms.length?l=this.itemDoms:l=this.itemDoms.slice(e,r);let d;if(n?d=yi(l.map(h=>h.context.key),n):i.length>l.length?d=[{op:"insert",index:l.length,count:i.length-l.length}]:i.length<l.length?d=[{op:"delete",index:i.length,count:l.length-i.length}]:d=[],d.length==0){C(this,b,Le).call(this,i,n,e,0,t);return}let c=[],m=[],f={insert:_,delete:v,store:P,restore:z},u=0;for(let h of d)h.index>u&&(C(this,b,Le).call(this,i,n,e+u,u,h.index-u),u=h.index),f[h.op].call(this,h);u<i.length&&C(this,b,Le).call(this,i,n,e+u,u,i.length-u),Xe(m),C(this,b,ft).call(this);function _(h){u+=h.count;let $=Math.min(m.length,h.count);if($){let L=m.splice(0,$);o(this,we).call(this,h.index+e,L),C(this,b,Le).call(this,i,n,h.index+e,h.index,$),o(this,J)&&ge(L,!0)}$<h.count&&o(this,$e).call(this,i,n,h.index+e+$,h.index+$,h.count-$)}function v(h){let $=o(this,be).call(this,h.index+e,h.count);o(this,J)&&ge($,!1),m.push(...$)}function P(h){c.push(...o(this,be).call(this,h.index+e,h.count))}function z(h){u+=h.count,o(this,we).call(this,h.index+e,c.slice(h.storeIndex,h.storeIndex+h.count)),C(this,b,Le).call(this,i,n,h.index+e,h.index,h.count)}},ft=function(){if(this.itemDoms.length==0)!this.emptyDom&&this.emptyConstructor&&(this.emptyDom=this.emptyConstructor(),o(this,b,ye)&&o(this,Z).before(...this.emptyDom.rootNodes),o(this,J)&&this.emptyDom.setMounted(!0)),this.emptyDom&&this.emptyDom.update();else if(this.emptyDom){if(o(this,b,ye))for(var e of this.emptyDom.rootNodes)e.remove();o(this,J)&&this.emptyDom.setMounted(!1),this.emptyDom.destroy(),this.emptyDom=null}},$e=new WeakMap,we=new WeakMap,Je=new WeakMap,be=new WeakMap,ye=function(){var e;return((e=o(this,Z))==null?void 0:e.parentNode)!=null},It=function(e,t,i,n,r){let l=[];for(let d=0;d<r;d++){let c={outer:this.outer,model:e[n+d],key:t==null?void 0:t[n+d],index:i+d};l.push(this.itemConstructor(c))}C(this,b,pt).call(this,i,l),o(this,J)&&ge(l,!0)},pt=function(e,t){if(this.itemDoms.splice(e,0,...t),o(this,b,ye)){let i=[];t.forEach(r=>i.push(...r.rootNodes));let n;e+t.length<this.itemDoms.length?n=this.itemDoms[e+t.length].rootNodes[0]:n=o(this,Z),n.before(...i)}},Bt=function(e,t){let i=C(this,b,mt).call(this,e,t);o(this,J)&&ge(i,!1),Xe(i)},mt=function(e,t){if(o(this,b,ye))for(let i=0;i<t;i++){let n=this.itemDoms[e+i].rootNodes;for(let r=0;r<n.length;r++)n[r].remove()}return this.itemDoms.splice(e,t)},Ot=function(e,t,i,n,r){let l=[];for(let d=0;d<r;d++){let c={outer:this.outer,model:e[n+d],key:t==null?void 0:t[n+d],index:i+d};l.push(this.itemConstructor(c))}C(this,b,gt).call(this,i,l),o(this,J)&&ge(l,!0)},gt=function(e,t){if(this.itemDoms.splice(e,0,...t),o(this,b,ye)){let i=t.map(r=>r.rootNode),n;e+t.length<this.itemDoms.length?n=this.itemDoms[e+t.length].rootNode:n=o(this,Z),n.before(...i)}},Ut=function(e,t){let i=C(this,b,yt).call(this,e,t);o(this,J)&&ge(i,!1),Xe(i)},yt=function(e,t){if(o(this,b,ye))for(let i=0;i<t;i++)this.itemDoms[e+i].rootNode.remove();return this.itemDoms.splice(e,t)},Le=function(e,t,i,n,r){for(let l=0;l<r;l++){let d=this.itemDoms[i+l];d.context.key=t==null?void 0:t[n+l],d.context.index=i+l,d.context.model=e[n+l],d.rebind(),d.update()}};let ut=Ke;function Xe(s){for(let e=s.length-1;e>=0;e--)s[e].destroy()}function ge(s,e){for(let t=s.length-1;t>=0;t--)s[t].setMounted(e)}Oe.register(ut);function Lt(s){let e=function(){var i;let t=(i=coenv.document)==null?void 0:i.createComment(s);return{get rootNode(){return t},get rootNodes(){return[t]},get isSingleRoot(){return!0},setMounted(n){},destroy(){},update(){}}};return e.isSingleRoot=!0,e}var He,de;const Ve=class Ve{constructor(e){w(this,He);w(this,de,!1);var t;this.isSingleRoot=e.data.isSingleRoot,this.branches=e.data.branches,this.key=e.data.key,this.branch_constructors=[],this.context=e.context;for(let i of this.branches)i.nodeIndex!==void 0?this.branch_constructors.push(e.nodes[i.nodeIndex]):this.branch_constructors.push(Lt(" IfBlock placeholder "));this.activeBranchIndex=-1,this.activeKey=void 0,this.activeBranch=Lt(" IfBlock placeholder ")(),this.isSingleRoot||(this.headSentinal=(t=coenv.document)==null?void 0:t.createComment(" if "))}static integrate(e){let t=[],i=e.key;delete e.key;let n=[],r=!1,l=!0;for(let d=0;d<e.branches.length;d++){let c=e.branches[d],m={};if(t.push(m),c.condition instanceof Function?(m.condition=c.condition,r=!1):c.condition!==void 0?(m.condition=()=>c.condition,r=!!c.condition):(m.condition=()=>!0,r=!0),c.template!==void 0){let f=new ne(c.template);f.isSingleRoot||(l=!1),m.nodeIndex=n.length,n.push(f)}}return delete e.branches,r||t.push({condition:()=>!0}),{isSingleRoot:l,nodes:n,data:{key:i,branches:t,isSingleRoot:l}}}static transform(e){if(e.key!==void 0){let t=e.key;if(!(t instanceof Function))throw new Error("`key` is not a function");return delete e.key,{type:Ve,key:t,branches:[{template:this.transform(e),condition:!0}]}}if(e.if!==void 0){let t={type:Ve,branches:[{template:e,condition:e.if}]};return delete e.if,t}return e}static transformGroup(e){let t=null,i=!1;for(let n=0;n<e.length;n++){let r=e[n];if(r.if)i||(e=[...e],i=!0),r=Object.assign({},r),t={type:Ve,branches:[{condition:r.if,template:r}]},delete r.if,e.splice(n,1,t);else if(r.elseif){if(!t)throw new Error("template has 'elseif' without a preceeding condition");r=Object.assign({},r),t.branches.push({condition:r.elseif,template:r}),delete r.elseif,e.splice(n,1),n--}else if(r.else!==void 0){if(!t)throw new Error("template has 'else' without a preceeding condition");r=Object.assign({},r),t.branches.push({condition:!0,template:r}),delete r.else,t=null,e.splice(n,1),n--}else t=null}return e}destroy(){this.activeBranch.destroy()}update(){this.switchActiveBranch(),this.activeBranch.update()}unbind(){var e,t;(t=(e=this.activeBranch).unbind)==null||t.call(e)}bind(){var e,t;(t=(e=this.activeBranch).bind)==null||t.call(e)}get isAttached(){var e;return this.isSingleRoot?((e=this.activeBranch.rootNode)==null?void 0:e.parentNode)!=null:this.headSentinal.parentNode!=null}switchActiveBranch(){var i,n,r,l,d;let e=this.resolveActiveBranch(),t=this.key?this.key.call(this.context.model,this.context.model,this.context):void 0;if(e!=this.activeBranchIndex||t!=this.activeKey){(i=o(this,He))==null||i.finish();let c=this.isAttached,m=this.activeBranch;if(this.activeBranchIndex=e,this.activeKey=t,this.activeBranch=this.branch_constructors[e](),c){let f;o(this,de)&&(this.key?f=(r=(n=this.key).withTransition)==null?void 0:r.call(n,this.context):f=(d=(l=this.branches[0].condition).withTransition)==null?void 0:d.call(l,this.context)),f||(f=Rt),g(this,He,f),f.enterNodes(this.activeBranch.rootNodes),f.leaveNodes(m.rootNodes),f.onWillEnter(()=>{this.isSingleRoot?m.rootNodes[m.rootNodes.length-1].after(this.activeBranch.rootNodes[0]):this.headSentinal.after(...this.activeBranch.rootNodes),o(this,de)&&this.activeBranch.setMounted(!0)}),f.onDidLeave(()=>{m.rootNodes.forEach(u=>u.remove()),o(this,de)&&m.setMounted(!1),m.destroy()}),f.start()}else o(this,de)&&(this.activeBranch.setMounted(!0),m.setMounted(!1),m.destroy())}}resolveActiveBranch(){for(let e=0;e<this.branches.length;e++)if(this.branches[e].condition.call(this.context.model,this.context.model,this.context))return e;throw new Error("internal error, IfBlock didn't resolve to a branch")}setMounted(e){g(this,de,e),this.activeBranch.setMounted(e)}get rootNodes(){return this.isSingleRoot?this.activeBranch.rootNodes:[this.headSentinal,...this.activeBranch.rootNodes]}get rootNode(){return this.activeBranch.rootNode}};He=new WeakMap,de=new WeakMap;let vt=Ve;Oe.register(vt);function vi(s,e){let t=1,i=1,n=[],r=null,l=new ne(s),d=new Map;return{code:m(l,!0).toString(),isSingleRoot:l.isSingleRoot,refs:n};function m(f,u){let _={emit_text_node:Zt,emit_html_node:Yt,emit_dynamic_text_node:Qt,emit_comment_node:Kt,emit_fragment_node:ii,emit_element_node:ni,emit_integrated_node:ei,emit_component_node:ti},v=new wt,P=v.addFunction("create").code,z=[],h=v.addFunction("bind").code,$=v.addFunction("update").code,L=v.addFunction("unbind").code,D=v.addFunction("setMounted",["mounted"]).code,R=v.addFunction("destroy").code,E=P.append,T=$.append,ue;u&&(ue=v.addFunction("rebind").code);let bt=new Map;u&&(r=v,r.code.append("let model = context.model;"),r.code.append("let document = env.document;")),v.code.append("create();"),v.code.append("bind();"),v.code.append("update();"),nt(f),E(z),h.closure.isEmpty||(E("bind();"),R.closure.addProlog().append("unbind();"));let fe=[];return f.isSingleRoot&&fe.push(`  get rootNode() { return ${f.spreadDomNodes()}; },`),u?(fe.push("  context,"),f==l&&d.forEach((a,y)=>fe.push(`  get ${y}() { return ${a}; },`)),v.getFunction("bind").isEmpty?ue.append("model = context.model"):(ue.append("if (model != context.model)"),ue.braced(()=>{ue.append("unbind();"),ue.append("model = context.model"),ue.append("bind();")})),fe.push("  rebind,")):(fe.push("  bind,"),fe.push("  unbind,")),v.code.append(["return { ","  update,","  destroy,","  setMounted,",`  get rootNodes() { return [ ${f.spreadDomNodes()} ]; },`,`  isSingleRoot: ${f.isSingleRoot},`,...fe,"};"]),v;function pe(a){a.template.export?r.addLocal(a.name):v.addLocal(a.name)}function je(){$.temp_declared||($.temp_declared=!0,T("let temp;"))}function nt(a){a.name=`n${t++}`,_[`emit_${a.kind}_node`](a)}function Zt(a){pe(a),E(`${a.name} = document.createTextNode(${JSON.stringify(a.template)});`)}function Yt(a){a.nodes.length!=0&&(pe(a),a.nodes.length==1?(E(`${a.name} = refs[${n.length}].cloneNode(true);`),n.push(a.nodes[0])):(E(`${a.name} = refs[${n.length}].map(x => x.cloneNode(true));`),n.push(a.nodes)))}function Qt(a){pe(a);let y=`p${i++}`;v.addLocal(y),E(`${a.name} = helpers.createTextNode("");`),je(),T(`temp = ${se(n.length)};`),T(`if (temp !== ${y})`),T(`  ${a.name} = helpers.setNodeText(${a.name}, ${y} = ${se(n.length)});`),n.push(a.template)}function Kt(a){if(pe(a),a.template.text instanceof Function){let y=`p${i++}`;v.addLocal(y),E(`${a.name} = document.createComment("");`),je(),T(`temp = ${se(n.length)};`),T(`if (temp !== ${y})`),T(`  ${a.name}.nodeValue = ${y} = temp;`),n.push(a.template.text)}else E(`${a.name} = document.createComment(${JSON.stringify(a.template.text)});`)}function ei(a){var S,B;(B=(S=a.integrated).compile)==null||B.call(S,e);let y=[],k=!1;if(a.integrated.nodes)for(let N=0;N<a.integrated.nodes.length;N++){let V=a.integrated.nodes[N];if(!V){y.push(null);continue}V.name=`n${t++}`;let Ee=m(V,!1);Ee.getFunction("bind").isEmpty||(k=!0);let Nt=`${V.name}_constructor_${N+1}`,ri=v.addFunction(Nt,[]);Ee.appendTo(ri.code),y.push(Nt)}T(`${a.name}.update()`),k&&(h.append(`${a.name}.bind()`),L.append(`${a.name}.unbind()`));let F=-1;a.integrated.data&&(F=n.length,n.push(a.integrated.data)),pe(a),E(`${a.name} = new refs[${n.length}]({`,"  context,",`  data: ${a.integrated.data?`refs[${F}]`:"null"},`,`  nodes: [ ${y.join(", ")} ],`,"});"),n.push(a.template.type),D.append(`${a.name}.setMounted(mounted);`),R.append(`${a.name}?.destroy();`),R.append(`${a.name} = null;`);for(let N of Object.keys(a.template))if(!st(a,N))throw new Error(`Unknown element template key: ${N}`)}function ti(a){pe(a),E(`${a.name} = new refs[${n.length}]();`),n.push(a.template.type);let y=new Set(a.template.type.slots??[]);y.size>0&&E(`${a.name}.create?.()`);let k=a.template.update==="auto",F=!1;D.append(`${a.name}.setMounted(mounted);`),R.append(`${a.name}?.destroy();`),R.append(`${a.name} = null;`);for(let S of Object.keys(a.template)){if(st(a,S)||S=="update")continue;if(y.has(S)){if(a.template[S]===void 0)continue;let N=new ne(a.template[S]);nt(N),N.isSingleRoot?E(`${a.name}${me(S)}.content = ${N.name};`):E(`${a.name}${me(S)}.content = [${N.spreadDomNodes()}];`);continue}let B=typeof a.template[S];if(B=="string"||B=="number"||B=="boolean")E(`${a.name}${me(S)} = ${JSON.stringify(a.template[S])}`);else if(B==="function"){k&&!F&&(F=`${a.name}_mod`,T(`let ${F} = false;`));let N=`p${i++}`;v.addLocal(N);let V=n.length;je(),T(`temp = ${se(V)};`),T(`if (temp !== ${N})`),k&&(T("{"),T(`  ${F} = true;`)),T(`  ${a.name}${me(S)} = ${N} = temp;`),k&&T("}"),n.push(a.template[S])}else{let N=a.template[S];N instanceof ui&&(N=N.value),E(`${a.name}${me(S)} = refs[${n.length}];`),n.push(N)}}a.template.update&&(typeof a.template.update=="function"?(T(`if (${se(n.length)})`),T(`  ${a.name}.update();`),n.push(a.template.update)):k?F&&(T(`if (${F})`),T(`  ${a.name}.update();`)):T(`${a.name}.update();`))}function ii(a){xt(a)}function ni(a){var F;let y=e.xmlns,k=a.template.xmlns;k===void 0&&a.template.type=="svg"&&(k="http://www.w3.org/2000/svg"),k==null&&(k=e.xmlns),pe(a),k?(e.xmlns=k,E(`${a.name} = document.createElementNS(${JSON.stringify(k)}, ${JSON.stringify(a.template.type)});`)):E(`${a.name} = document.createElement(${JSON.stringify(a.template.type)});`),R.append(`${a.name} = null;`);for(let S of Object.keys(a.template)){if(st(a,S))continue;if(S.startsWith("class_")){let N=at(S.substring(6)),V=a.template[S];if(V instanceof Function){a.bcCount||(a.bcCount=0);let Ee=`${a.name}_bc${a.bcCount++}`;v.addLocal(Ee),E(`${Ee} = helpers.boolClassMgr(context, ${a.name}, ${JSON.stringify(N)}, refs[${n.length}]);`),n.push(V),T(`${Ee}();`)}else E(`helpers.setNodeClass(${a.name}, ${JSON.stringify(N)}, ${V});`);continue}if(S.startsWith("style_")){let N=at(S.substring(6));rt(a.template[S],V=>`helpers.setNodeStyle(${a.name}, ${JSON.stringify(N)}, ${V})`);continue}if(S=="display"){if(a.template.display instanceof Function){let N=`${a.name}_dm`;v.addLocal(N),E(`${N} = helpers.displayMgr(context, ${a.name}, refs[${n.length}]);`),n.push(a.template.display),T(`${N}();`)}else E(`helpers.setNodeDisplay(${a.name}, ${JSON.stringify(a.template.display)});`);continue}if(S=="text"){a.template.text instanceof Function?rt(a.template.text,N=>`helpers.setElementText(${a.name}, ${N})`):a.template.text instanceof I&&E(`${a.name}.innerHTML = ${JSON.stringify(a.template.text.html)};`),typeof a.template.text=="string"&&E(`${a.name}.textContent = ${JSON.stringify(a.template.text)};`);continue}let B=S;S.startsWith("attr_")&&(B=B.substring(5)),e.xmlns||(B=at(B)),rt(a.template[S],N=>`helpers.setElementAttribute(${a.name}, ${JSON.stringify(B)}, ${N})`)}xt(a),(F=a.childNodes)!=null&&F.length&&E(`${a.name}.append(${a.spreadChildDomNodes()});`),e.xmlns=y}function xt(a){if(a.childNodes)for(let y=0;y<a.childNodes.length;y++)nt(a.childNodes[y])}function st(a,y){if(si(y))return!0;if(y=="export"){if(typeof a.template.export!="string")throw new Error("'export' must be a string");if(d.has(a.template.export))throw new Error(`duplicate export name '${a.template.export}'`);return d.set(a.template.export,a.name),!0}if(y=="bind"){if(typeof a.template.bind!="string")throw new Error("'bind' must be a string");if(bt.has(a.template.export))throw new Error(`duplicate bind name '${a.template.bind}'`);return bt.set(a.template.bind,!0),h.append(`model${me(a.template.bind)} = ${a.name};`),L.append(`model${me(a.template.bind)} = null;`),!0}if(y.startsWith("on_")){let k=y.substring(3),F=a.template[y];if(typeof F=="string"){let B=F;F=(N,...V)=>N[B](...V)}if(!(F instanceof Function))throw new Error(`event handler for '${y}' is not a function`);a.listenerCount||(a.listenerCount=0),a.listenerCount++;let S=`${a.name}_ev${a.listenerCount}`;return v.addLocal(S),E(`${S} = helpers.addEventListener(() => context, ${a.name}, ${JSON.stringify(k)}, refs[${n.length}]);`),n.push(F),R.append(`${S}?.();`),R.append(`${S} = null;`),!0}if(y=="input"){let k=`${a.name}_in`;return v.addLocal(k),E(`${k} = helpers.input(refs[${n.length}])`),z.push(`${k}.create(${a.name}, context);`),n.push(a.template[y]),T(`${k}.update()`),R.append(`${k}.destroy()`),!0}return y=="debug_create"?(typeof a.template[y]=="function"?(E(`if (${se(n.length)})`),E("  debugger;"),n.push(a.template[y])):a.template[y]&&E("debugger;"),!0):y=="debug_update"?(typeof a.template[y]=="function"?(T(`if (${se(n.length)})`),T("  debugger;"),n.push(a.template[y])):a.template[y]&&T("debugger;"),!0):y=="debug_render"}function si(a){return a=="type"||a=="childNodes"||a=="xmlns"}function se(a){return`refs[${a}].call(model, model, context)`}function rt(a,y){if(a instanceof Function){let k=`p${i++}`;v.addLocal(k),y(),je(),T(`temp = ${se(n.length)};`),T(`if (temp !== ${k})`),T(`  ${y(k+" = temp")};`),n.push(a)}else E(y(JSON.stringify(a)))}}}let $i=1;function At(s,e){e||(e={});let t=Object.assign({},e,{compileTemplate:At}),i=vi(s,t),n=new Function("env","refs","helpers","context",i.code),r=function(l){return l||(l={}),l.$instanceId=$i++,n(coenv,i.refs,Me,l??{})};return r.isSingleRoot=i.isSingleRoot,r}var U,Pe,xe,ce,q,Ne;const re=class re extends EventTarget{constructor(){super();w(this,U);w(this,Pe,!1);w(this,xe,null);w(this,ce,0);w(this,q);w(this,Ne,!1);this.update=this.update.bind(this),this.invalidate=this.invalidate.bind(this)}static get domTreeConstructor(){return this._domTreeConstructor||(this._domTreeConstructor=this.onProvideDomTreeConstructor()),this._domTreeConstructor}static onProvideDomTreeConstructor(){try{return this._compiling=this.onProvideTemplate(),At(this._compiling)}finally{delete this._compiling}}static onProvideTemplate(){return this.template}static get isSingleRoot(){return this._compiling?new ne(this._compiling).isSingleRoot:this.domTreeConstructor.isSingleRoot}create(){o(this,U)||g(this,U,new this.constructor.domTreeConstructor({model:this}))}get created(){return o(this,U)!=null}get domTree(){return o(this,U)||this.create(),o(this,U)}get isSingleRoot(){return this.domTree.isSingleRoot}get rootNode(){if(!this.isSingleRoot)throw new Error("rootNode property can't be used on multi-root template");return this.domTree.rootNode}get rootNodes(){return this.domTree.rootNodes}get invalid(){return o(this,Pe)}invalidate(){o(this,U)&&(this.invalid||(g(this,Pe,!0),re.invalidateWorker(this)))}validate(){this.invalid&&this.update()}static invalidateWorker(t){this._invalidComponents.push(t),this._invalidComponents.length==1&&tt(()=>{for(let i=0;i<this._invalidComponents.length;i++)this._invalidComponents[i].validate();this._invalidComponents=[]},re.nextFrameOrder)}update(){o(this,U)&&(g(this,Pe,!1),this.domTree.update())}get loadError(){return o(this,xe)}set loadError(t){g(this,xe,t),this.invalidate()}get loading(){return o(this,ce)!=0}async load(t,i){if(i){let n=await t();return this.invalidate(),n}_e(this,ce)._++,o(this,ce)==1&&(g(this,xe,null),this.invalidate(),coenv.enterLoading(),this.dispatchEvent(new Event("loading")));try{return await t()}catch(n){g(this,xe,n)}finally{_e(this,ce)._--,o(this,ce)==0&&(this.invalidate(),this.dispatchEvent(new Event("loaded")),coenv.leaveLoading())}}destroy(){o(this,U)&&(o(this,U).destroy(),g(this,U,null))}onMount(){}onUnmount(){}listen(t,i,n){!t||!i||(n||(n=this.invalidate),o(this,q)||g(this,q,[]),o(this,q).push({target:t,event:i,handler:n}),o(this,Ne)&&t.addEventListener(i,n))}unlisten(t,i,n){if(!t||!i||!o(this,q))return;let r=o(this,q).findIndex(l=>l.target==t&&l.event==i&&l.handler==n);r>=0&&(o(this,q).splice(r,1),o(this,Ne)&&t.removeEventListener(i,n))}get mounted(){return o(this,Ne)}setMounted(t){var n;(n=o(this,U))==null||n.setMounted(t),g(this,Ne,t);let i=!1;t&&o(this,q)&&(o(this,q).forEach(r=>r.target.addEventListener(r.event,r.handler)),i=o(this,q).length>0&&o(this,U)),t?this.onMount():this.onUnmount(),i&&this.invalidate(),!t&&o(this,q)&&o(this,q).forEach(r=>r.target.removeEventListener(r.event,r.handler))}mount(t){coenv.mount(this,t)}unmount(){coenv.unmount(this)}};U=new WeakMap,Pe=new WeakMap,xe=new WeakMap,ce=new WeakMap,q=new WeakMap,Ne=new WeakMap,M(re,"_domTreeConstructor"),M(re,"nextFrameOrder",-100),M(re,"_invalidComponents",[]),M(re,"template",{});let Q=re;class wi{constructor(e){this.$node={type:e}}append(...e){e=e.flat(1e3).map(t=>t.$node??t),this.$node.$?this.$node.$.push(...e):this.$node.$=[...e]}attr(e,t){if((e=="class"||e=="style")&&arguments.length>2?(e=`${e}_${t}`,t=arguments[2]):e=="on"&&arguments.length>2?(e=`on_${t}`,t=arguments[2]):e=="type"&&(e="attr_type"),this.$node[e]!==void 0)throw new Error(`duplicate attribute: ${e}`);this.$node[e]=t}}let bi={get:function(s,e){if(e!="bind"&&e!="name"){let t=Reflect.get(s,e);if(t!==void 0)return t}return(...t)=>(Reflect.get(s,"$tb").attr(e,...t),Reflect.get(s,"$proxy"))}},xi={get:function(s,e){let t=Reflect.get(s,e);return t!==void 0?t:it(e)}};function it(s){let e=new wi(s),t,i=function(){return e.append(...arguments),t};return i.$tb=e,i.$node=e.$node,t=new Proxy(i,bi),i.$proxy=t,t}it.html=Dt;it.encode=Ze;let p=new Proxy(it,xi);class Ni extends ci{constructor(){super(),this.browser=!0,this.document=document,this.window=window,this.hydrateMounts=document.head.querySelector("meta[name='co-ssr']")?[]:null,this.pendingStyles=""}declareStyle(e){e.length>0&&(this.pendingStyles+=`
`+e,this.hydrateMounts||this.window.requestAnimationFrame(()=>this.mountStyles()))}mountStyles(){this.pendingStyles.length!=0&&(this.styleNode||(this.styleNode=document.createElement("style")),this.styleNode.innerHTML+=this.pendingStyles+`
`,this.pendingStyles="",this.styleNode.parentNode||document.head.appendChild(this.styleNode))}doHydrate(){tt(async()=>{for(;await this.untilLoaded(),pi();)await new Promise(e=>Et(e));Et(()=>{document.querySelectorAll(".cossr").forEach(t=>t.remove());let e=this.hydrateMounts;this.hydrateMounts=null,e.forEach(t=>{Tt(t.el),this.mount(t.component,t.el)}),Tt(document.head),this.mountStyles()})},Number.MAX_SAFE_INTEGER)}mount(e,t){typeof t=="string"&&(t=document.querySelector(t)),this.hydrateMounts?(this.hydrateMounts.push({el:t,component:e}),this.hydrateMounts.length==1&&this.doHydrate()):(t.append(...e.rootNodes),e.setMounted(!0))}unmount(e){e.created&&e.rootNodes.forEach(t=>t.remove()),e.setMounted(!1)}async fetchTextAsset(e){let t=await fetch(e);if(!t.ok)throw new Error(`Failed to fetch '${e}': ${t.status} ${t.statusText}`);return t.text()}}if(typeof document<"u"){let s=new Ni;hi(()=>s)}function Tt(s){let e=s.firstChild,t=!1;for(;e;){let i=e.nextSibling;e.nodeType==8&&e.data=="co-ssr-start"&&(t=!0),t&&e.remove(),e.nodeType==8&&e.data=="co-ssr-end"&&(t=!1),e=i}}function Si(s){let e="^",t=s.length,i;for(let r=0;r<t;r++){i=!0;let l=s[r];if(l=="?")e+="[^\\/]";else if(l=="*")r+1==t?e+="(?<tail>.*)":e+="[^\\/]+";else if(l==":"){r++;let d=r;for(;r<t&&n(s[r]);)r++;let c=s.substring(d,r);if(c.length==0)throw new Error("syntax error in url pattern: expected id after ':'");let m="[^\\/]+";if(s[r]=="("){r++,d=r;let f=0;for(;r<t;){if(s[r]=="(")f++;else if(s[r]==")"){if(f==0)break;f--}r++}if(r>=t)throw new Error("syntax error in url pattern: expected ')'");m=s.substring(d,r),r++}if(r<t&&s[r]=="*"||s[r]=="+"){let f=s[r];r++,s[r]=="/"?(e+=`(?<${c}>(?:${m}\\/)${f})`,r++):f=="*"?e+=`(?<${c}>(?:${m}\\/)*(?:${m})?\\/?)`:e+=`(?<${c}>(?:${m}\\/)*(?:${m})\\/?)`,i=!1}else e+=`(?<${c}>${m})`;r--}else l=="/"?(e+="\\"+l,r==s.length-1&&(e+="?")):".$^{}[]()|*+?\\/".indexOf(l)>=0?(e+="\\"+l,i=l!="/"):e+=l}return i&&(e+="\\/?"),e+="$",e;function n(r){return r>="a"&&r<="z"||r>="A"&&r<="Z"||r>="0"&&r<="9"||r=="_"||r=="$"}}class kt{static get(){if(coenv.browser)return{top:coenv.window.pageYOffset||coenv.document.documentElement.scrollTop,left:coenv.window.pageXOffset||coenv.document.documentElement.scrollLeft}}static set(e){coenv.browser&&(e?coenv.window.scrollTo(e.left,e.top):coenv.window.scrollTo(0,0))}}var te,j;class Ei{constructor(e){w(this,te);w(this,j,{});g(this,te,e),coenv.window.history.scrollRestoration&&(coenv.window.history.scrollRestoration="manual");let t=coenv.window.sessionStorage.getItem("codeonly-view-states");t&&g(this,j,JSON.parse(t)),e.addEventListener("mayLeave",(i,n)=>(this.captureViewState(),!0)),e.addEventListener("mayEnter",(i,n)=>{n.navMode!="push"&&(n.viewState=o(this,j)[n.state.sequence])}),e.addEventListener("didEnter",(i,n)=>{if(n.navMode=="push"){for(let r of Object.keys(o(this,j)))parseInt(r)>n.state.sequence&&delete o(this,j)[r];this.saveViewStates()}Mt(coenv,()=>{tt(()=>{var r,l;if(n.handler.restoreViewState?n.handler.restoreViewState(n.viewState,n):o(this,te).restoreViewState?(l=(r=o(this,te)).restoreViewState)==null||l.call(r,n.viewState,n):kt.set(n.viewState),coenv.browser){let d=document.getElementById(n.url.hash.substring(1));d==null||d.scrollIntoView()}})})}),coenv.window.addEventListener("beforeunload",i=>{this.captureViewState()})}captureViewState(){var t,i;let e=o(this,te).current;e&&(e.handler.captureViewState?o(this,j)[e.state.sequence]=e.handler.captureViewState(e):o(this,te).captureViewState?o(this,j)[e.state.sequence]=(i=(t=o(this,te)).captureViewState)==null?void 0:i.call(t,e):o(this,j)[e.state.sequence]=kt.get()),this.saveViewStates()}saveViewStates(){coenv.window.sessionStorage.setItem("codeonly-view-states",JSON.stringify(o(this,j)))}}te=new WeakMap,j=new WeakMap;var Fe,A,Ie;class _i{constructor(){w(this,Fe,0);w(this,A);w(this,Ie,!1)}async start(e){g(this,A,e),new Ei(e),coenv.document.body.addEventListener("click",r=>{if(r.defaultPrevented)return;let l=r.target.closest("a");if(l){if(l.hasAttribute("download"))return;let d=l.getAttribute("href"),c=new URL(d,coenv.window.location);if(c.origin==coenv.window.location.origin){try{c=o(this,A).internalize(c)}catch{return}return this.navigate(c).then(m=>{m==null&&(window.location.href=d)}),r.preventDefault(),!0}}}),coenv.window.addEventListener("popstate",async r=>{if(o(this,Ie)){g(this,Ie,!1);return}let l=o(this,Fe)+1,d=o(this,A).internalize(new URL(coenv.window.location)),c=r.state??{sequence:this.current.state.sequence+1};await this.load(d,c,{navMode:"pop"})||l==o(this,Fe)&&(g(this,Ie,!0),coenv.window.history.go(this.current.state.sequence-c.sequence))});let t=o(this,A).internalize(new URL(coenv.window.location)),i=coenv.window.history.state??{sequence:0},n=await this.load(t,i,{navMode:"start"});return coenv.window.history.replaceState(i,null),n}get current(){return o(this,A).current}async load(e,t,i){return _e(this,Fe)._++,await o(this,A).load(e,t,i)}back(){if(this.current.state.sequence==0){let e=new URL("/",o(this,A).internalize(new URL(coenv.window.location))),t={sequence:0};coenv.window.history.replaceState(t,"",o(this,A).externalize(e)),this.load(e,t,{navMode:"replace"})}else coenv.window.history.back()}replace(e){typeof e=="string"&&(e=new URL(e,o(this,A).internalize(new URL(coenv.window.location)))),e!==void 0&&(this.current.pathname=e.pathname,this.current.url=e,e=o(this,A).externalize(e).href),coenv.window.history.replaceState(this.current.state,"",e)}async navigate(e){typeof e=="string"&&(e=new URL(e,o(this,A).internalize(new URL(coenv.window.location))));let t=await this.load(e,{sequence:this.current.state.sequence+1},{navMode:"push"});return t&&(coenv.window.history.pushState(t.state,"",o(this,A).externalize(e)),t)}}Fe=new WeakMap,A=new WeakMap,Ie=new WeakMap;var Se,x,$t,et,Te,X,zt,ve,Ye,ze,he,Be;class Li{constructor(e){w(this,x);M(this,"urlMapper");w(this,Se);M(this,"urlMapper");w(this,et,{c:null,p:null,l:[]});w(this,he,[]);w(this,Be,!1);M(this,"captureViewState");M(this,"restoreViewState");e&&this.register(e)}start(e){if(!o(this,Se))return e||(e=new _i),g(this,Se,e),e&&(this.navigate=e.navigate.bind(e),this.replace=e.replace.bind(e),this.back=e.back.bind(e)),o(this,Se).start(this)}internalize(e){return C(this,x,$t).call(this,e,"internalize")}externalize(e){return C(this,x,$t).call(this,e,"externalize")}get current(){return o(this,x,X)}get pending(){return o(this,x,ve)}addEventListener(e,t){o(this,x,ze).push({event:e,handler:t})}removeEventListener(e,t){let i=o(this,x,ze).findIndex(n=>n.event==e&&n.handler==t);i>=0&&o(this,x,ze).splice(i,1)}async dispatchEvent(e,t,i,n){for(let r of o(this,x,ze))if(r.event==e){let l=r.handler(i,n);if(t&&await Promise.resolve(l)==!1)return!1}return!0}async load(e,t,i){return coenv.load(async()=>{var r,l,d;i=i??{};let n=o(this,x,X);if(((r=o(this,x,X))==null?void 0:r.url.pathname)==e.pathname&&o(this,x,X).url.search==e.search){let c=(d=(l=o(this,x,X).handler).hashChange)==null?void 0:d.call(l,o(this,x,X),i);c!==void 0?i=c:i=Object.assign({},o(this,x,X),i)}if(i=Object.assign(i,{current:!1,url:new URL(e),state:t}),g(this,x,i,Ye),!i.match&&(i=await this.matchUrl(e,t,i),!i))return null;try{await this.tryLoad(i)!==!0&&g(this,x,null,Ye)}catch(c){throw this.dispatchCancelEvents(n,i),c}return o(this,x,ve)!=i?(this.dispatchCancelEvents(n,i),null):(g(this,x,null,Ye),i)})}dispatchCancelEvents(e,t){var i,n,r,l,d;(r=(i=o(this,x,X))==null?void 0:(n=i.handler).cancelLeave)==null||r.call(n,e,t),(d=(l=t.handler).cancelEnter)==null||d.call(l,e,t),this.dispatchEvent("cancel",!1,e,t)}async tryLoad(e){var n,r,l,d,c,m,f,u;let t=o(this,x,X),i;if(!(t&&(!await this.dispatchEvent("mayLeave",!0,t,e)||e!=o(this,x,ve)||(i=(r=(n=t.handler).mayLeave)==null?void 0:r.call(n,t,e),await Promise.resolve(i)===!1)||e!=o(this,x,ve)))&&(i=(d=(l=e.handler).mayEnter)==null?void 0:d.call(l,t,e),await Promise.resolve(i)!==!1&&e==o(this,x,ve)&&await this.dispatchEvent("mayEnter",!0,t,e)&&e==o(this,x,ve)))return t&&(t.current=!1),e.current=!0,g(this,x,e,zt),t&&(this.dispatchEvent("didLeave",!1,t,e),(m=t==null?void 0:(c=t.handler).didLeave)==null||m.call(c,t,e)),(u=(f=e.handler).didEnter)==null||u.call(f,t,e),this.dispatchEvent("didEnter",!1,t,e),!0}async matchUrl(e,t,i){o(this,Be)&&(o(this,he).sort((n,r)=>(n.order??0)-(r.order??0)),g(this,Be,!1));for(let n of o(this,he))if(!(n.pattern&&(i.match=i.url.pathname.match(n.pattern),!i.match)))if(n.match){let r=await Promise.resolve(n.match(i));if(r===!0||r==i)return i.handler=n,i;if(r===null)return null}else return i.handler=n,i;return i.handler={},i}register(e){Array.isArray(e)||(e=[e]);for(let t of e)typeof t.pattern=="string"&&(t.pattern=new RegExp(Si(t.pattern))),o(this,he).push(t);g(this,Be,!0)}revoke(e){g(this,he,o(this,he).filter(t=>!e(t)))}}Se=new WeakMap,x=new WeakSet,$t=function(e,t){return this.urlMapper?e instanceof URL?this.urlMapper[t](e):this.urlMapper[t](new URL(e,"http://x/")).href.substring(8):e instanceof URL?new URL(e):e},et=new WeakMap,Te=function(){var e;return((e=o(this,Se))==null?void 0:e.state)??o(this,et)},X=function(){return o(this,x,Te).c},zt=function(e){o(this,x,Te).c=e},ve=function(){return o(this,x,Te).p},Ye=function(e){o(this,x,Te).p=e},ze=function(){return o(this,x,Te).l},he=new WeakMap,Be=new WeakMap;let G=new Li,Y={appName:"irtx",description:"irtx IR Blaster",deviceUrl:""};class Vt extends Q{}M(Vt,"template",{type:"header #header",$:[{type:"a .title",href:"/",$:[{type:"img",src:"/public/logo.svg"},Y.appName]},{type:"nav .nav-links",$:[{type:"a",attr_href:"/",text:"Status"},{type:"a",attr_href:"/dmesg",text:"Log"},{type:"a",attr_href:"/activities",text:"Activities"},{type:"a",attr_href:"/console",text:"Console"}]},{type:"div .buttons",$:[{type:"input type=checkbox .theme-switch",on_click:()=>window.stylish.toggleTheme()},{type:"script",text:Dt('document.querySelector(".theme-switch").checked = window.stylish.darkMode;')}]}]});Ue`
:root
{
    --header-height: 50px;
}

#header
{
    position: fixed;
    top: 0;
    width: 100%;
    height: var(--header-height);

    display: flex;
    justify-content: start;
    align-items: center;
    border-bottom: 1px solid var(--gridline-color);
    padding-left: 10px;
    padding-right: 10px;
    background-color: rgb(from var(--back-color) r g b / 75%);
    z-index: 1;

    .title 
    {
        flex-grow: 1;
        display: flex;
        align-items: center;
        color: var(--body-fore-color);
        transition: opacity 0.2s;

        &:hover
        {
            opacity: 75%;
        }

        img
        {
            height: calc(var(--header-height) - 25px);
            padding-right: 10px
        }
    }


    .nav-links
    {
        display: flex;
        gap: 20px;
        margin-right: 20px;

        a
        {
            color: var(--body-fore-color);
            text-decoration: none;
            font-size: 0.9rem;
            opacity: 0.7;
            transition: opacity 0.15s;

            &:hover
            {
                opacity: 1;
            }
        }
    }

    .buttons
    {
        font-size: 12pt;
        display: flex;
        gap: 10px;
        align-items: center;

        .theme-switch
        {
            transform: translateY(-1.5px);
        }
    }
}
`;class qt extends Q{constructor(){super(),G.addEventListener("didEnter",(e,t)=>{this.invalidate(),coenv.browser&&(document.title=this.title)})}get title(){var e;return(e=G.current)!=null&&e.title?`${G.current.title} - ${Y.appName}`:Y.appName}get name(){return Y.appName}get description(){return Y.description}get url(){var e;return((e=G.current)==null?void 0:e.url.href)??""}get image(){return null}}M(qt,"template",[p.title(e=>e.title),p.meta.name("description").content(e=>e.description),p.meta.itemprop("name").content(e=>e.name),p.meta.itemprop("description").content(e=>e.description),p.meta.itemprop("image").content(e=>e.image).if(e=>e.image),p.meta.name("og:url").content(e=>e.url),p.meta.name("og:type").content("website"),p.meta.name("og:title").content(e=>e.title),p.meta.name("og:description").content(e=>e.description),p.meta.name("og:image").content(e=>e.image).if(e=>e.image),p.meta.name("twitter:card").content("summary_large_image"),p.meta.name("twitter:title").content(e=>e.title),p.meta.name("twitter:description").content(e=>e.description),p.meta.name("twitter:image").content(e=>e.image).if(e=>e.image),p.link.rel("apple-touch-icon").href(e=>e.image).if(e=>e.image)]);class Wt extends Q{constructor(){super(...arguments);M(this,"status",null);M(this,"busy",!1)}onMount(){this.loadStatus()}async loadStatus(){await this.load(async()=>{let t=await fetch(`${Y.deviceUrl}/api/status`);if(!t.ok)throw new Error(`HTTP ${t.status}`);this.status=await t.json()})}async sendCommand(t){if(!this.busy){this.busy=!0,this.invalidate();try{let i=await fetch(`${Y.deviceUrl}/api/command`,{method:"POST",body:t});if(!i.ok)throw new Error(`HTTP ${i.status}`)}finally{this.busy=!1}await this.loadStatus()}}makeActive(t){this.sendCommand(`activity ${t}`)}makeDefault(t){this.sendCommand(`setdefact ${t}`)}formatPins(t){return!t||t.length===0?"none":t.map(i=>Array.isArray(i)?`encoder(${i[0]}, ${i[1]})`:String(i)).join(", ")}}M(Wt,"template",{type:"main",class:"home-page",$:[{if:t=>t.loading,type:"div",class:"status-loading",text:"Loading…"},{elseif:t=>t.loadError,type:"div",class:"status-error",text:t=>`Failed to load status: ${t.loadError.message}`},{elseif:t=>t.status,type:"div",class:"status-sections",$:[p.h2("Device"),{type:"table",class:"kv-table",$:[{type:"tr",$:[p.th("Name"),{type:"td",text:t=>t.status.device.name}]},{type:"tr",$:[p.th("Logging"),{type:"td",text:t=>t.status.device.logging}]}]},p.h2("WiFi"),{type:"table",class:"kv-table",$:[{type:"tr",$:[p.th("Mode"),{type:"td",text:t=>t.status.wifi.mode}]},{type:"tr",$:[p.th("SSID"),{type:"td",text:t=>t.status.wifi.ssid}]},{type:"tr",$:[p.th("Connected"),{type:"td",text:t=>t.status.wifi.connected?"Yes":"No"}]},{type:"tr",$:[p.th("IP"),{type:"td",text:t=>t.status.wifi.ip}]},{type:"tr",$:[p.th("Gateway"),{type:"td",text:t=>t.status.wifi.gateway}]},{type:"tr",$:[p.th("RSSI"),{type:"td",text:t=>`${t.status.wifi.rssi} dBm`}]},{type:"tr",$:[p.th("MAC"),{type:"td",text:t=>t.status.wifi.mac}]},{type:"tr",$:[p.th("UDP Port"),{type:"td",text:t=>String(t.status.wifi.udp_port)}]}]},p.h2("GPIO"),{type:"table",class:"kv-table",$:[{type:"tr",$:[p.th("IR TX Pin"),{type:"td",text:t=>String(t.status.gpio.irtx_pin)}]},{type:"tr",$:[p.th("IR RX Pin"),{type:"td",text:t=>t.status.gpio.irrx_pin===-1?"disabled":String(t.status.gpio.irrx_pin)}]},{type:"tr",$:[p.th("LED Pin"),{type:"td",text:t=>String(t.status.gpio.led_pin)}]},{type:"tr",$:[p.th("LED Order"),{type:"td",text:t=>t.status.gpio.led_order}]},{type:"tr",$:[p.th("Pullup"),{type:"td",text:t=>t.formatPins(t.status.gpio.pullup)}]},{type:"tr",$:[p.th("Pulldown"),{type:"td",text:t=>t.formatPins(t.status.gpio.pulldown)}]}]},p.h2("Protocols"),{type:"table",class:"data-table",$:[{type:"thead",$:{type:"tr",$:[p.th("Name"),p.th("ID"),p.th("Bits")]}},{type:"tbody",$:{foreach:t=>t.status.protocols,type:"tr",$:[{type:"td",text:t=>t.name},{type:"td",text:t=>t.id},{type:"td",text:t=>String(t.bits)}]}}]},p.h2("BLE"),{type:"table",class:"data-table",$:[{type:"thead",$:{type:"tr",$:[p.th("Slot"),p.th("ID"),p.th("Peer")]}},{type:"tbody",$:{foreach:t=>t.status.ble,type:"tr",$:[{type:"td",text:t=>String(t.slot)},{type:"td",text:t=>t.id},{type:"td",text:t=>t.peer??"—"}]}}]},p.h2("Activities"),{type:"table",class:"data-table",$:[{type:"thead",$:{type:"tr",$:[p.th("#"),p.th("Name"),p.th("Active"),p.th("Default"),p.th("")]}},{type:"tbody",$:{foreach:t=>t.status.activities.list,type:"tr",$:[{type:"td",text:t=>String(t.index)},{type:"td",text:t=>t.name},{type:"td",text:t=>t.active?"✓":""},{type:"td",text:t=>t.default?"✓":""},{type:"td",class:"activity-actions",$:[{if:t=>!t.active,type:"button",class:"action-btn",text:"Make Active",on_click:(t,i,n)=>n.outer.model.makeActive(n.index)},{if:t=>!t.active&&!t.default,type:"span",text:" | "},{if:t=>!t.default,type:"button",class:"action-btn",text:"Make Default",on_click:(t,i,n)=>n.outer.model.makeDefault(n.index)}]}]}}]}]}]});Ue`
.home-page
{
    padding: 20px 30px;
    max-width: 800px;

    h2
    {
        margin: 1.5rem 0 0.4rem;
        font-size: 0.8rem;
        text-transform: uppercase;
        letter-spacing: 0.08em;
        opacity: 0.6;
        border-bottom: 1px solid var(--gridline-color);
        padding-bottom: 0.25rem;
    }

    .kv-table,
    .data-table
    {
        width: 100%;
        border-collapse: collapse;
        font-size: 0.9rem;

        th, td
        {
            padding: 4px 12px 4px 0;
            text-align: left;
            border-bottom: 1px solid var(--gridline-color);
        }
    }

    .kv-table
    {
        th
        {
            font-weight: normal;
            opacity: 0.6;
            white-space: nowrap;
            width: 1%;
            padding-right: 24px;
        }
    }

    .data-table
    {
        thead th
        {
            font-weight: 600;
            padding-right: 24px;
        }

        td
        {
            padding-right: 24px;
        }
    }

    .activity-actions
    {
    }

    .action-btn
    {
        background: none;
        border: none;
        padding: 0;
        font-size: 0.8rem;
        color: var(--accent-color, #4af);
        cursor: pointer;
        opacity: 0.8;

        &:hover
        {
            opacity: 1;
            text-decoration: underline;
        }
    }

    .status-loading
    {
        margin-top: 3rem;
        opacity: 0.5;
    }

    .status-error
    {
        margin-top: 2rem;
        color: var(--danger-color, #c00);
    }
}
`;G.register({pattern:"/",match:s=>(s.page=new Wt,!0)});class Jt extends Q{constructor(){super(...arguments);M(this,"log",null)}onMount(){this.load(async()=>{let t=await fetch(`${Y.deviceUrl}/api/dmesg`);if(!t.ok)throw new Error(`HTTP ${t.status}`);this.log=await t.text()})}}M(Jt,"template",{type:"main",class:"dmesg-page",$:[{if:t=>t.loading,type:"div",class:"dmesg-loading",text:"Loading…"},{elseif:t=>t.loadError,type:"div",class:"dmesg-error",text:t=>`Failed to load log: ${t.loadError.message}`},{elseif:t=>t.log!==null,type:"pre",class:"dmesg-output",text:t=>t.log}]});Ue`
.dmesg-page
{
    padding: 20px 30px;

    .dmesg-output
    {
        font-family: monospace;
        font-size: 0.85rem;
        white-space: pre-wrap;
        word-break: break-all;
        margin: 0;
    }

    .dmesg-loading
    {
        opacity: 0.5;
    }

    .dmesg-error
    {
        color: var(--danger-color, #c00);
    }
}
`;G.register({pattern:"/dmesg",match:s=>(s.page=new Jt,!0)});class Ht extends Q{constructor(){super(...arguments);M(this,"entries",[]);M(this,"busy",!1)}async runCommand(t){this.entries.push({kind:"command",text:t}),this.busy=!0,this.update(),this.outputEl.scrollTop=this.outputEl.scrollHeight;try{let i=await fetch(`${Y.deviceUrl}/api/command`,{method:"POST",body:t}),n=await i.text();i.ok?this.entries.push({kind:"response",text:n}):this.entries.push({kind:"error",text:`Error ${i.status}: ${n}`})}catch(i){this.entries.push({kind:"error",text:i.message})}this.busy=!1,this.update(),this.outputEl.scrollTop=this.outputEl.scrollHeight}onKeyDown(t){if(t.key!=="Enter"||this.busy)return;let i=this.cmdInput.value.trim();i&&(this.cmdInput.value="",this.runCommand(i))}}M(Ht,"template",{type:"div",class:"console-page",$:[{type:"div",class:"console-output",bind:"outputEl",$:{foreach:{items:t=>t.entries,itemKey:(t,i)=>i.index},type:"pre",class:t=>`console-entry console-${t.kind}`,text:t=>t.kind==="command"?`> ${t.text}`:t.text}},{type:"div",class:"console-input-bar",$:{type:"input type=text",class:"console-input",bind:"cmdInput",placeholder:"Enter command…",attr_disabled:t=>t.busy||void 0,on_keydown:"onKeyDown"}}]});Ue`
.console-page
{
    display: flex;
    flex-direction: column;
    height: calc(100vh - var(--header-height));

    .console-output
    {
        flex: 1;
        overflow-y: auto;
        padding: 12px 16px;

        .console-entry
        {
            margin: 0;
            font-family: monospace;
            font-size: 0.85rem;
            white-space: pre-wrap;
            word-break: break-all;
            line-height: 1.5;
        }

        .console-command
        {
            color: var(--accent-color, #4af);
        }

        .console-response
        {
            color: var(--body-fore-color);
        }

        .console-error
        {
            color: var(--danger-color, #c00);
        }
    }

    .console-input-bar
    {
        border-top: 1px solid var(--gridline-color);
        padding: 8px 12px;

        .console-input
        {
            width: 100%;
            font-family: monospace;
            font-size: 0.9rem;
            background: transparent;
            border: none;
            outline: none;
            color: var(--body-fore-color);
            box-sizing: border-box;

            &::placeholder
            {
                opacity: 0.4;
            }

            &:disabled
            {
                opacity: 0.5;
            }
        }
    }
}
`;let dt=null;G.register({pattern:"/console",match:s=>(dt||(dt=new Ht),s.page=dt,!0)});class jt extends Q{constructor(){super(...arguments);M(this,"busy",!1);M(this,"result",null)}get selectedFile(){var t;return((t=this.fileInput)==null?void 0:t.files[0])??null}async upload(){if(!(this.busy||!this.selectedFile)){this.busy=!0,this.result=null,this.invalidate();try{let t=new FormData;t.append("file",this.selectedFile);let i=await fetch(`${Y.deviceUrl}/api/activities`,{method:"POST",body:t});this.result=i.ok?{ok:!0,message:"Upload successful."}:{ok:!1,message:`Upload failed: HTTP ${i.status}`}}catch(t){this.result={ok:!1,message:`Upload failed: ${t.message}`}}this.busy=!1,this.invalidate()}}onSubmit(t){t.preventDefault(),this.upload()}}M(jt,"template",{type:"main",class:"activities-page",$:[p.h1("Upload Activities"),p.p("Select a compiled activities.bin file to upload to the device."),{type:"form",on_submit:"onSubmit",$:[{type:"div",class:"field",$:[p.label.attr_for("file-input").text("File"),{type:"input type=file",id:"file-input",attr_accept:".bin",bind:"fileInput"}]},{type:"button type=submit",class:"upload-btn",attr_disabled:t=>t.busy||void 0,text:t=>t.busy?"Uploading…":"Upload"}]},{if:t=>t.result!==null,type:"div",class:t=>{var i;return`upload-result ${(i=t.result)!=null&&i.ok?"ok":"error"}`},text:t=>{var i;return(i=t.result)==null?void 0:i.message}}]});Ue`
.activities-page
{
    padding: 20px 30px;
    max-width: 600px;

    h1
    {
        font-size: 1.2rem;
        margin-bottom: 0.5rem;
    }

    p
    {
        opacity: 0.7;
        font-size: 0.9rem;
        margin-bottom: 1.5rem;
    }

    .field
    {
        display: flex;
        flex-direction: column;
        gap: 6px;
        margin-bottom: 1rem;

        label
        {
            font-size: 0.8rem;
            text-transform: uppercase;
            letter-spacing: 0.08em;
            opacity: 0.6;
        }
    }

    .upload-btn
    {
        padding: 6px 20px;
        cursor: pointer;

        &:disabled
        {
            opacity: 0.5;
            cursor: default;
        }
    }

    .upload-result
    {
        margin-top: 1rem;
        font-size: 0.9rem;

        &.ok    { color: var(--success-color, #4a4); }
        &.error { color: var(--danger-color,  #c00); }
    }
}
`;G.register({pattern:"/activities",match:s=>(s.page=new jt,!0)});class Gt extends Q{constructor(e){super(),this.url=e}}M(Gt,"template",{type:"div",class:"center",$:[{type:"h1",class:"danger",text:"Page not found! 😟"},{type:"p",text:e=>`The page ${e.url} doesn't exist!`},{type:"p",$:{type:"a",href:"/",text:"Return Home"}}]});G.register({match:s=>(s.page=new Gt(s.url),!0),order:1e3});class Xt extends Q{constructor(){super(),this.page=null,G.addEventListener("didEnter",(e,t)=>{t.page&&(this.page=t.page,this.invalidate())})}}M(Xt,"template",{type:"div",$:[Vt,{type:"div #layoutRoot",$:{type:"embed-slot",content:e=>e.page}}]});Ue`
#layoutRoot
{
    padding-top: var(--header-height);
}

`;function Ti(){new qt().mount("head"),new Xt().mount("body"),G.start()}Ti();
