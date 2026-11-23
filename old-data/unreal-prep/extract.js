const fs=require('fs');const s=fs.readFileSync('/home/claude/draw-fight/index.html','utf8');
function lit(name){const i=s.indexOf('const '+name+'=');if(i<0)throw name;let j=s.indexOf('=',i)+1;const st=j;let d=0,q=null;for(;j<s.length;j++){const c=s[j];if(q){if(c==='\\'){j++;continue;}if(c===q)q=null;continue;}if(c==="'"||c==='"'||c==='`'){q=c;continue;}if('[{('.includes(c))d++;else if(']})'.includes(c)){d--;if(d===0){j++;break;}}}return s.slice(st,j);}
function ev(name){return Function('return ('+lit(name)+')')();}
// all Object.assign(X,{...}) extensions
function assigns(name){const out=[];let i=0;const key='Object.assign('+name+',';while((i=s.indexOf(key,i))>=0){let j=i+key.length;const st=j;let d=0,q=null;for(;j<s.length;j++){const c=s[j];if(q){if(c==='\\'){j++;continue;}if(c===q)q=null;continue;}if(c==="'"||c==='"'||c==='`'){q=c;continue;}if('[{('.includes(c))d++;else if(']})'.includes(c)){d--;if(d===0){j++;break;}}}out.push(Function('return ('+s.slice(st,j)+')')());i=j;}return out;}
const D={};
for(const n of ['SKILL','ELEM','CLASS','PERK','PASSIVE','RAR','SIGM','SIGA','SIGE','SIGKW','SIGCLS','KIND','GA_GRP','GA_EL','MOVES','WTYPES','WT2C','SKICON','HDBOSS','BEATS','EN']){try{D[n]=ev(n);}catch(e){console.log('skip',n,String(e).slice(0,80));}}
for(const n of ['SKILL','CLASS','ELEM','PERK']){for(const a of assigns(n))Object.assign(D[n],a);}
const m=s.match(/const GA_COST=(\d+),GA_REF=(\[[^\]]*\])/);D.GA={cost:+m[1],ten:+m[1]*9,refund:JSON.parse(m[2]),pity:30};
const pm=s.match(/pity[^;]{0,120}/);console.log('pity ctx:',pm&&pm[0]);
fs.writeFileSync('raw.json',JSON.stringify(D,(k,v)=>typeof v==='function'?undefined:v,1));
for(const k in D)console.log(k,Array.isArray(D[k])?D[k].length:Object.keys(D[k]).length);
