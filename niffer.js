class funcNode {
    name;
    func;
    args;

    constructor( name, func, args ) {
        this.name = name;
        this.func = func;
        this.args = args;   // must be array
    }

    jenString() {
        let out = this.name + " =\t" + this.func + "( ";
        out += this.args.join(", ");
        out += " )\n";
        return out;
    }
} 

class funcTree {
    name;
    n;
    nodes;

    constructor( name ) {
        this.name = name;
        this.n = 0;
        this.nodes = [];
    }

    addNode( node ) {
        this.n++;
        this.nodes.push( node );
    }

    // Generate a warp function that does nothing
    generateIdentityWarp() {  
        this.addNode( new funcNode( "position", "vect2", [ 0.0, 0.0 ] ) );
        this.addNode( new funcNode( "result", "fn_v_copy", [ "position" ] ) );
    } 

    jenString() {
        let out = "\n*****\n\nname =\t" + this.name + "\n";
        out += "n = \t" + this.n + "\n";
        for( const node of this.nodes ) {
            out += node.jenString();
        }
        out += "\n*****\n";
        return out;
    }
}

function identityWarp() {
    let identiTree = new funcTree( "identity_warp" );
    identiTree.generateIdentityWarp();
    console.log( identiTree.jenString() );
}