// ExpressionParser.cc

#include "parse.h"
#include <cmath>
#include <iomanip>

namespace utils
{
  //////////////////////////////////////////////////

  template<typename T>
  ExpressionParser<T>::ExpressionParser()
  {
    init_op();
  }
  
  template<typename T>
  ExpressionParser<T>::ExpressionParser(gcore::System * sys,
					bound::Boundary * pbc)
    : d_value_traits(sys, pbc)
  {
    init_op();
  }

  template<typename T>
  void ExpressionParser<T>::init_op()
  {
    d_op_string = "*/+-,=!><&|?:";

    d_op["sin"] = op_sin;
    d_op["cos"] = op_cos;
    d_op["tan"] = op_tan;
    d_op["asin"] = op_asin;
    d_op["acos"] = op_acos;
    d_op["atan"] = op_atan;
    d_op["exp"] = op_exp;
    d_op["ln"] = op_ln;
    d_op["abs"] = op_abs;
    d_op["abs2"] = op_abs2;
    d_op["dot"] = op_dot;
    d_op["cross"] = op_cross;
    d_op["ni"] = op_ni;

    d_op["*"] = op_mul;
    d_op["/"] = op_div;
    d_op["+"] = op_add;
    d_op["-"] = op_sub;
    
    d_op["!"] = op_not;
    d_op["=="] = op_eq;
    d_op["!="] = op_neq;
    d_op["<"] = op_less;
    d_op[">"] = op_greater;
    d_op["<="] = op_lesseq;
    d_op[">="] = op_greatereq;
    d_op["&&"] = op_and;
    d_op["||"] = op_or;

    d_op["?"] = op_condask;
    d_op[":"] = op_condition;
  }
  
  template<typename T>
  T ExpressionParser<T>::parse_expression(std::string s,
					  std::map<std::string, T> & var)
  {
    std::string::size_type it = 0;
    std::vector<expr_struct> expr;

    _parse_token(op_undef, s, it, var, expr);
    try{
      T res = calculate(expr, var);
      return res;
    }
    catch(std::string n){
      throw std::runtime_error("Variable '" + n + "' unknown!");
    }
  }

  template<typename T>
  void ExpressionParser<T>::parse_expression(std::string s,
					     std::map<std::string, T> & var,
					     std::vector<expr_struct> & expr)
  {
    std::string::size_type it = 0;
    _parse_token(op_undef, s, it, var, expr);

    // print_expression(expr, std::cerr);
  }

  template<typename T>
  void ExpressionParser<T>::print_expression(std::vector<expr_struct> & expr,
					     std::ostream & os)
  {
    for(unsigned int i=0; i < expr.size(); ++i){
      switch(expr[i].type){
	case expr_value:
	  os << "value     \t" << expr[i].value << "\n";
	  break;
	case expr_operator:
	  os << "operator  \t" << expr[i].op << "\n";
	  break;
	case expr_function:
	  os << "function  \t" << expr[i].op << "\n";
	  break;
	case expr_variable:
	  os << "variable  \t" << expr[i].name << "\n";
	  break;
	default:
	  os << "error\n";
      }
    }
  }

  template<typename T>
  void ExpressionParser<T>::_parse_token(operation_enum op,
					 std::string s,
					 std::string::size_type & it,
					 std::map<std::string, T> & var,
					 std::vector<expr_struct> & expr)
  {
    // function, unary operator, value
    _parse_value(s, it, var, expr);
    
    operation_enum op2 = op_undef;
    if (it != std::string::npos)
      op2 = _parse_operator(s, it);
    // else std::cerr << "no second operator: end" << std::endl;
    
    if (op <= op2){
      if (op != op_undef)
	_commit_operator(op, expr);
      if (op2 != op_undef)
	_parse_token(op2, s, it, var, expr);
      // else std::cerr << "end reached" << std::endl;
    }
    else
      {
	_parse_token(op2, s, it, var, expr);
	if (op != op_undef)_commit_operator(op, expr);
      }
  }

  template<typename T>
  bool ExpressionParser<T>::_parse_function
  (
   std::string s, 
   std::string::size_type & it,
   std::map<std::string, T> & var,
   std::vector<expr_struct> & expr
   )
  {
    // std::cerr << "parsing function " 
    // << s.substr(it, std::string::npos) << std::endl;
    
    std::string::size_type bra = s.find_first_not_of(" ", it);
    operation_enum fop = op_undef;
    
    bool found = false;
    
    if (s[bra] == '('){
      bra += 1;
      found = true;
    }
    else{
      std::map<std::string,operation_enum>::const_iterator
	it = d_op.begin(),
	to = d_op.end();
      
      for( ; it!=to; ++it){
	std::string::size_type len = it->first.length() + 1;
	if (s.substr(bra, len) == it->first + "("){
	  bra += len;
	  fop = it->second;
	  found = true;
	  break;
	}
      }
    }
    if (!found){
      return false;
    }
    
    std::string::size_type ket = find_matching_bracket(s, '(', bra);
    if (ket == std::string::npos){
      throw std::runtime_error("could not find matching bracket");
    }

    std::string::size_type fit = 0;
    _parse_token(op_undef, s.substr(bra, ket-bra-1), fit, var, expr);

    if (fop != op_undef){
      _commit_operator(fop, expr);
    }
    
    if (s.length() > ket)
      it = ket;
    else it = std::string::npos;
    
    return true;
  }
  
  template<typename T>
  operation_enum
  ExpressionParser<T>::_parse_unary_operator
  (
   std::string s, 
   std::string::size_type & it
   )
  {
    // std::cerr << "parsing unary: " << s.substr(it, std::string::npos)
    // << std::endl;

    std::string::size_type bra = s.find_first_not_of(" ", it);
    if (bra == std::string::npos) return op_undef;
  
    // std::cerr << "\tunary: " << s[bra] << std::endl;
  
    if (s[bra] == '-'){ it = bra + 1; return op_umin; }
    if (s[bra] == '+'){ it = bra + 1; return op_uplus; }   

    return op_undef;
  }

  template<typename T>
  operation_enum
  ExpressionParser<T>::_parse_operator
  (
   std::string s, 
   std::string::size_type & it
   )
  {
    // std::cerr << "parse_operator: " << s.substr(it, std::string::npos)
    // << std::endl;
    
    std::string::size_type bra = s.find_first_of(d_op_string, it);
    if (bra == std::string::npos) return op_undef;
    
    // check if its a two character operator
    int op_len = 1;
    std::string::size_type bra2 = s.find_first_of(d_op_string, bra+1);
    if (bra2 == bra+1) op_len = 2;

    std::string ops = s.substr(bra, op_len);
    // std::cerr << "ops: " << ops << std::endl;
    
    if (d_op.count(ops) == 0)
      throw std::runtime_error("operator undefined! (" + ops + ")");

    it+= op_len;
    return d_op[ops];
  }

  template<typename T>
  void ExpressionParser<T>::_parse_value
  (
   std::string s, 
   std::string::size_type & it,
   std::map<std::string, T> & var,
   std::vector<expr_struct> & expr
   )
  {
    if (!_parse_function(s, it, var, expr)){

      operation_enum u_op = op_undef;
      u_op = _parse_unary_operator(s, it);
      
      if (u_op != op_undef){
	_parse_value(s, it, var, expr);
	_commit_operator(u_op, expr);
	return;
      }
      
      _commit_value(s, it, var, expr);
    }
    
    // try if it's a list of values
    std::string::size_type com = s.find_first_not_of(" ", it);
    
    if (com != std::string::npos &&
	s[com] == ','){

      it = com+1;
      if (it > s.length()) throw std::runtime_error("comma: empty list");
      
      _parse_value(s, it, var, expr);
    }
  }
  
  template<typename T>
  void ExpressionParser<T>::_commit_value
  (
   std::string s, 
   std::string::size_type & it,
   std::map<std::string, T> & var,
   std::vector<expr_struct> & expr
   )
  {
    std::string::size_type vit = find_par(s, d_op_string, it, "(", ")");

    // std::cerr << "_commit_value: " << name << " from " 
    // << s.substr(it, vit-it) << " and original " << s << std::endl;
    
    try{
      expr_struct e(d_value_traits.parseValue(s.substr(it, vit - it), var));
      expr.push_back(e);
    }
    catch(std::runtime_error e){
      std::istringstream is(s.substr(it, vit-it));
      std::string name;
      is >> name;
      expr_struct e(name);
      expr.push_back(e);
    }
    
    it = vit;
    if (it >=  s.length()) it = std::string::npos;
  }

  template<typename T>
  void ExpressionParser<T>::_commit_operator
  (
   operation_enum op,
   std::vector<expr_struct> & expr
   )
  {
    // std::cerr << "committing operator " << op << std::endl;
    expr.push_back(expr_struct(op));
  }

  template<typename T>
  T ExpressionParser<T>::calculate
  (
   std::vector<expr_struct> & expr,
   std::map<std::string, T> & var
   )
  {
    for(unsigned int i=0; i<expr.size(); ++i){
      switch(expr[i].type){
	case expr_value: d_stack.push(expr[i].value); break;
	case expr_variable: {
	  if (var.count(expr[i].name) == 0)
	    throw std::string(expr[i].name);
	  d_stack.push(var[expr[i].name]);
	  break;
	}
	case expr_function: do_function(expr[i].op); break;
	case expr_operator: do_operation(expr[i].op); break;
	default: throw std::runtime_error("wrong expression type");
      }
    }
    
    if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
    T res = d_stack.top();
    d_stack.pop();
    return res;
  }

  template<typename T>
  void ExpressionParser<T>::calculate
  (
   std::string name,
   std::map<std::string, std::vector<expr_struct> > & expr,
   std::map<std::string, T> & var
   )
  {
    if (var.count(name) == 0){
      
      while(true){

	try{
	  T res = calculate(expr[name], var);
	  var[name] = res;
	  break;
	}
	catch(std::string n){
	  if (n == name)
	    throw std::runtime_error("Implicit expression for '" + n + "'");
	  
	  if (expr.count(n) == 0)
	    throw std::runtime_error("No expression to calculate '" + n + "'");
	  
	  calculate(n, expr, var);
	}
      }
    }
  }


  template<typename T>
  void ExpressionParser<T>::calculate
  (
   std::map<std::string, std::vector<expr_struct> > & expr,
   std::map<std::string, T> & var
   )
  {
    typename std::map<std::string, std::vector<expr_struct> >::const_iterator
      it = expr.begin(),
      to = expr.end();
    
    for( ; it != to; ++it){
      
	calculate(it->first, expr, var);
    }
  }


  template<typename T>
  T ExpressionParser<T>::do_operation(operation_enum op)
  {
    T res;
    if (op < op_unary) throw std::runtime_error("operator is function");
    if (op < op_binary){
      if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
      T arg = d_stack.top();
      d_stack.pop();
      switch(op){
	case op_uplus: res = arg; break;
	case op_umin: res = -arg; break;
	default: throw std::runtime_error("unknown unary operator");
      }
    }
    else if (op < op_logical){
      if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
      T arg2 = d_stack.top();
      d_stack.pop();
      if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
      T arg1 = d_stack.top();
      d_stack.pop();
      switch(op){
	case op_add: res = arg1 + arg2; break;
	case op_sub: res = arg1 - arg2; break;
	case op_mul: res = arg1 * arg2; break;
	case op_div: res = arg1 / arg2; break;
	default: throw std::runtime_error("unknown binary operator");
      }
    }
    else if (op < op_undef){
      // delegate the logical operators, not everything might
      // be defined with those...
      ValueTraits<T>::do_operation(op, *this);
      return d_stack.top();
    }
    else{
      throw std::runtime_error("unknown / undef operator");
    }
    
    d_stack.push(res);
    return res;
  }

  template<typename T>
  void ExpressionParser<T>::do_function(operation_enum op)
  {
    ValueTraits<T>::do_function(op, *this);
  }

  template<typename T>
  void ExpressionParser<T>::do_logical_operation(operation_enum op)
  {
    if (op == op_undef) return;
    
    if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");

    if (op == op_not){
      T arg = d_stack.top();
      T res = ! arg;
      op = op_undef;
      d_stack.pop();
      d_stack.push(res);
      return;
    }

    if (op < op_ternary){
      T arg2 = d_stack.top();
      d_stack.pop();
      if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
      T arg1 = d_stack.top();
      d_stack.pop();
      
      // std::cerr << "arg1 = " << arg1 
      // << " arg2 = " << arg2
      // << " op = " << op
      // << std::endl;

      T res;
      switch(op){
	case op_eq:        res = (arg1 == arg2); break;
	case op_neq:       res = (arg1 != arg2); break;
	case op_less:      res = (arg1 < arg2); break;
	case op_greater:   res = (arg1 > arg2); break;
	case op_lesseq:    res = (arg1 <= arg2); break;
	case op_greatereq: res = (arg1 >= arg2); break;
	case op_and:       res = (arg1 && arg2); break;
	case op_or:        res = (arg1 || arg2); break;
	default: return;
      }

      // std::cerr << "res = " << res << std::endl;
      
      op = op_undef;
      d_stack.push(res);
      return;
    }

    if (op == op_condask){
      // do nothing! leave the arguments for the op_condition
      return;
    }
    
    if (op == op_condition){
      T arg3 = d_stack.top();
      d_stack.pop();
      if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
      T arg2 = d_stack.top();
      d_stack.pop();
      if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
      T arg1 = d_stack.top();
      d_stack.pop();
      
      T res = (arg1 == true) ? arg2 : arg3;
      op = op_undef;
      d_stack.push(res);
      return;
    }
  }
  
  template<typename T>
  void ExpressionParser<T>::do_general_function(operation_enum op)
  {
    if (op == op_undef) return;
    
    T res;
    if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
    T arg = d_stack.top();
    
    switch(op){
      case op_abs: res = T(abs(arg)); break;
      default: return;
    }
    
    op = op_undef;
    d_stack.pop();
    d_stack.push(res);
  }
  
  template<typename T>
  void ExpressionParser<T>::do_trigonometric_function(operation_enum op)
  {
    if (op == op_undef) return;
    
    T res;
    if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
    T arg = d_stack.top();

    switch(op){
      case op_sin: res = sin(arg); break;
      case op_cos: res = cos(arg); break;
      case op_tan: res = tan(arg); break;
      case op_asin: res = asin(arg); break;
      case op_acos: res = acos(arg); break;
      case op_atan: res = atan(arg); break;
	
      default: return;
    }
    
    op = op_undef;
    d_stack.pop();
    d_stack.push(res);
  }

  template<typename T>
  void ExpressionParser<T>::do_transcendent_function(operation_enum op)
  {
    if (op == op_undef) return;
    
    T res;
    if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
    T arg = d_stack.top();
    
    switch(op){
      case op_exp: res = exp(arg); break;
      case op_ln:  res = log(arg); break;
      default: return;
    }
    
    op = op_undef;
    d_stack.pop();
    d_stack.push(res);
  }

  template<typename T>
  void ExpressionParser<T>::do_vector_function(operation_enum op)
  {
    if (op == op_undef) return;
    
    T res;
    
    switch(op){
      case op_abs2:
	{
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg = d_stack.top();
	  d_stack.pop();
	  res = abs2(arg);
	  break;
	}
      case op_dot:
	{
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg1 = d_stack.top();
	  d_stack.pop();
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg2 = d_stack.top();
	  d_stack.pop();
	  res = dot(arg1, arg2);
	  break;
	}
      case op_cross:
	{
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg1 = d_stack.top();
	  d_stack.pop();
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg2 = d_stack.top();
	  d_stack.pop();
	  res = cross(arg1, arg2);
	  break;
	}
      case op_ni:
	{
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg1 = d_stack.top();
	  d_stack.pop();
	  if (d_stack.empty()) throw std::runtime_error("too few arguments on stack");
	  T arg2 = d_stack.top();
	  d_stack.pop();
	  res = Value(d_value_traits.pbc()->nearestImage
		      (arg1.vec(), arg2.vec(), d_value_traits.sys()->box()));
	  break;
	}
      default: return;
    }

    op = op_undef;
    d_stack.push(res);
  }
}
