(function(global){
  "use strict";

  function normalize(value){
    return String(value || "").trim().toUpperCase().replace(/[^A-Z0-9\/]/g, "");
  }

  function addAliases(aliasMap, canonical, aliases){
    var i;
    for(i = 0; i < aliases.length; i++){
      aliasMap[String(aliases[i]).toUpperCase()] = canonical;
    }
  }

  var aliasMap = {};

  addAliases(aliasMap, "K", ["AA","AB","AC","AD","AE","AF","AG","AI","AJ","AK","AL","K","N","W"]);
  addAliases(aliasMap, "VE", ["CF","CG","CH","CI","CJ","CK","CY","CZ","VA","VB","VC","VD","VE","VF","VG","VO","VX","VY","XJ","XK","XL","XM","XN","XO"]);
  addAliases(aliasMap, "VK", ["AX","VH","VI","VJ","VK","VL","VM","VN","VZ"]);
  addAliases(aliasMap, "JA", ["7J","7K","7L","7M","7N","8J","8K","8L","8M","8N","JA","JB","JC","JD","JE","JF","JG","JH","JI","JJ","JK","JL","JM","JN","JO","JP","JQ","JR","JS"]);
  addAliases(aliasMap, "PY", ["PP","PQ","PR","PS","PT","PU","PV","PW","PX","PY","ZV","ZW","ZX","ZY","ZZ"]);
  addAliases(aliasMap, "LU", ["AY","AZ","LO","LP","LQ","LR","LS","LT","LU","LV","LW"]);
  addAliases(aliasMap, "CE", ["3G","CA","CB","CC","CD","CE","XQ","XR"]);
  addAliases(aliasMap, "CX", ["CV","CW","CX"]);
  addAliases(aliasMap, "OA", ["4T","OA","OB","OC"]);
  addAliases(aliasMap, "HK", ["5J","5K","HJ","HK"]);
  addAliases(aliasMap, "HC", ["HC","HD"]);
  addAliases(aliasMap, "YV", ["4M","YV","YW","YY"]);
  addAliases(aliasMap, "4X", ["4X","4Z"]);
  addAliases(aliasMap, "HP", ["3E","3F","H3","H8","H9","HO","HP"]);
  addAliases(aliasMap, "TG", ["TD","TG"]);
  addAliases(aliasMap, "HR", ["HQ","HR"]);
  addAliases(aliasMap, "YN", ["H6","H7","HT","YN"]);
  addAliases(aliasMap, "CO", ["CL","CM","CO","T4"]);
  addAliases(aliasMap, "XE", ["4A","4B","4C","6D","6E","6F","6G","6H","6I","6J","XA","XB","XC","XD","XE","XF","XG","XH","XI"]);
  addAliases(aliasMap, "TI", ["TE","TI"]);
  addAliases(aliasMap, "KH2", ["AH2","KH2"]);
  addAliases(aliasMap, "DL", ["DA","DB","DC","DD","DE","DF","DG","DH","DJ","DK","DL","DM","DN","DO","DP","DQ","DR","Y2","Y3","Y4","Y5","Y6","Y7","Y8","Y9"]);
  addAliases(aliasMap, "ON", ["ON","OO","OP","OQ","OR","OS","OT"]);
  addAliases(aliasMap, "PA", ["PA","PB","PC","PD","PE","PF","PG","PH","PI"]);
  addAliases(aliasMap, "F", ["F","FA","FB","FC","FD","FE","FF","HW","HX","HY","TH","TM","TO","TP","TQ","TV","TX"]);
  addAliases(aliasMap, "EA", ["AM","AN","AO","EA","EB","EC","ED","EE","EF","EG","EH"]);
  addAliases(aliasMap, "CT", ["CQ","CR","CS","CT"]);
  addAliases(aliasMap, "OZ", ["5P","5Q","OU","OV","OW","OZ"]);
  addAliases(aliasMap, "SM", ["7S","8S","SA","SB","SC","SD","SE","SF","SG","SH","SI","SJ","SK","SL","SM"]);
  addAliases(aliasMap, "LA", ["LA","LB","LC","LD","LE","LF","LG","LH","LI","LJ","LK","LM","LN"]);
  addAliases(aliasMap, "OH", ["OF","OG","OH","OI"]);
  addAliases(aliasMap, "SP", ["3Z","HF","SN","SO","SP","SQ","SR"]);
  addAliases(aliasMap, "HB", ["HB","HE"]);
  addAliases(aliasMap, "I", ["I","IA","IB","IC","ID","IE","IF","IG","II","IJ","IK","IL","IM","IN","IO","IQ","IR","IS","IT","IU","IV","IW","IX","IY","IZ"]);
  addAliases(aliasMap, "SV", ["J4","SV","SX","SY","SZ"]);
  addAliases(aliasMap, "TA", ["TA","TB","TC","YM"]);
  addAliases(aliasMap, "YO", ["YO","YP","YQ","YR"]);
  addAliases(aliasMap, "HA", ["HA","HG"]);
  addAliases(aliasMap, "OK", ["OK","OL"]);
  addAliases(aliasMap, "EI", ["EI","EJ"]);
  addAliases(aliasMap, "YU", ["4N","4O","YT","YU","YZ"]);
  addAliases(aliasMap, "UR", ["EM","EN","EO","UR","US","UT","UU","UV","UW","UX","UY","UZ"]);
  addAliases(aliasMap, "EU", ["EU","EV","EW"]);
  addAliases(aliasMap, "ZS", ["ZS","ZT","ZU"]);
  addAliases(aliasMap, "BY", ["BA","BD","BG","BH","BI","BJ","BL","BM","BN","BO","BP","BQ","BR","BT","BU","BY","XS"]);
  addAliases(aliasMap, "HL", ["6K","6L","6M","6N","D7","D8","D9","DS","DT","HL"]);
  addAliases(aliasMap, "YB", ["7A","7B","7C","7D","7E","7F","7G","7H","7I","8A","8B","8C","8D","8E","8F","8G","8H","8I","YB","YC","YD","YE","YF","YG","YH"]);
  addAliases(aliasMap, "DU", ["4D","4E","4F","4G","4H","4I","DU","DV","DW","DX","DY","DZ"]);
  addAliases(aliasMap, "HS", ["E2","HS"]);
  addAliases(aliasMap, "VU", ["8T","8U","8V","8W","8X","8Y","AT","AU","AV","AW","VT","VU","VW"]);
  addAliases(aliasMap, "AP", ["AP","AS"]);
  addAliases(aliasMap, "SU", ["6A","6B","SS","SU"]);
  addAliases(aliasMap, "CN", ["5C","5D","CN"]);
  addAliases(aliasMap, "7X", ["7R","7T","7U","7V","7W","7X"]);
  addAliases(aliasMap, "5H", ["5H","5I"]);
  addAliases(aliasMap, "5Z", ["5Y","5Z"]);
  addAliases(aliasMap, "5B", ["5B","H2","P3"]);
  addAliases(aliasMap, "EL", ["5L","5M","A8","EL"]);
  addAliases(aliasMap, "HH", ["4V","HH"]);
  addAliases(aliasMap, "YS", ["HU","YS"]);
  addAliases(aliasMap, "YK", ["6C","YK"]);
  addAliases(aliasMap, "E7", ["E7","T9"]);
  addAliases(aliasMap, "ET", ["9E","9F","ET"]);
  addAliases(aliasMap, "EP", ["9B","9C","9D","EP"]);
  addAliases(aliasMap, "HZ", ["7Z","HZ"]);
  addAliases(aliasMap, "ST", ["6T","6U","ST"]);
  addAliases(aliasMap, "A2", ["8O","A2"]);
  addAliases(aliasMap, "KL", ["AL7","KL","NL7","WL7"]);
  addAliases(aliasMap, "OX", ["OX","XP"]);

  var aliasPrefixes = Object.keys(aliasMap).sort(function(a, b){
    return b.length - a.length;
  });
  var specialExactPrefixes = ["JD1/M","JD1/O","EA6","EA8","EA9","CT3","CU","IS0","3B9"];

  function resolveSpecialCases(normalized){
    var match;
    var i;
    if(!normalized){
      return "";
    }

    for(i = 0; i < specialExactPrefixes.length; i++){
      if(normalized.indexOf(specialExactPrefixes[i]) === 0){
        return specialExactPrefixes[i];
      }
    }

    if(/^(?:A|N|W)H[0-9]/.test(normalized)){
      return "KH" + normalized.charAt(2);
    }
    if(/^L[2-9]/.test(normalized)){
      return "LU";
    }

    if(normalized.indexOf("GI") === 0 || normalized.indexOf("2I") === 0 || normalized.indexOf("MI") === 0){ return "GI"; }
    if(normalized.indexOf("GM") === 0 || normalized.indexOf("2M") === 0 || normalized.indexOf("MM") === 0){ return "GM"; }
    if(normalized.indexOf("GW") === 0 || normalized.indexOf("2W") === 0 || normalized.indexOf("MW") === 0){ return "GW"; }
    if(normalized.indexOf("GD") === 0 || normalized.indexOf("2D") === 0 || normalized.indexOf("MD") === 0){ return "GD"; }
    if(normalized.indexOf("GJ") === 0 || normalized.indexOf("2J") === 0 || normalized.indexOf("MJ") === 0){ return "GJ"; }
    if(normalized.indexOf("GU") === 0 || normalized.indexOf("2U") === 0 || normalized.indexOf("MU") === 0){ return "GU"; }
    if(normalized.indexOf("M") === 0 || normalized.indexOf("2E") === 0 || normalized.indexOf("GB") === 0 || normalized.indexOf("GX") === 0 || normalized.indexOf("MX") === 0){ return "G"; }

    if(/^(R[0-9]|R[A-Z][0-9]|U[A-I][0-9]|U[A-I][A-Z][0-9])/.test(normalized)){
      match = normalized.match(/[0-9]/);
      if(normalized.indexOf("UA2") === 0){
        return "UA2";
      }
      return match && /^[089]$/.test(match[0]) ? "UA/AS" : "UA/E";
    }
    return "";
  }

  function resolveAlias(value){
    var normalized = normalize(value);
    var special = resolveSpecialCases(normalized);
    var i;
    var prefix;
    if(!normalized){
      return "";
    }
    if(special){
      return special;
    }
    for(i = 0; i < aliasPrefixes.length; i++){
      prefix = aliasPrefixes[i];
      if(normalized.indexOf(prefix) === 0){
        return aliasMap[prefix];
      }
    }
    return normalized;
  }

  global.normalizeDxccAliasInput = normalize;
  global.canonicalizeDxccPrefixAlias = resolveAlias;
})(window);
