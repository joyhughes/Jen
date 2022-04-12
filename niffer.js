const calculateFunction = document.querySelector('.calculateFunction');

// Returns +1 or -1 at random
function posOrNeg() {
    return Math.floor( Math.random() * 2 ) * 2 - 1;
}

function fairCoin() {
    return Math.random() < 0.5;
}

function randomRange( min, max ) {
    return min + Math.random() * ( max - min );
}

function randomAngle()  {
    return Math.random() * Math.pi * 2.0;
}

// min and max must be arrays of same length
function boxOfRandom( min, max ) {
    let out = [];
    for( let i = 0; i < min.length; i++ ) {
        out.push( randomRange( min[i], max[i] ) );
    }
    return out;
}

// RotationDirection enums can be grouped as static members of a class
class RotationDirection {
    // Create new instances of the same class as static attributes
    static Counterclockwise = new RotationDirection( "counterclockwise ")
    static Clockwise        = new RotationDirection( "clockwise" )
    static Random           = new RotationDirection( "random" )
    static LavaLamp         = new RotationDirection( "lavaLamp" )
  
    constructor(name) {
      this.name = name
    }
}

class FuncNode {
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


class Vortex {
    tag;        // Appended to each node name generated
    diameter;   // float - Overall size of vortex
    soften;     // float - Avoids a singularity in the center of vortex
    intensity;  // float - Strength of vortex. How vortexy is it? Negative value swirls the opposite direction.
    centerOrig; // vect2 - Initial position of vortex
    // other mathematical properties here

    // Animation parameters
    revolving;  // bool - does the vortex revolve around a center?
    velocity;   // float - Speed of revolution. Must be integer for animation to loop
    centerOfRevolution;   // vect2 - vortex revolves around this point

    constructor( tag, diameter, soften, intensity, centerOrig ) {
        this.tag = tag;
        this.diameter = diameter;
        this.soften = soften;
        this.intensity = intensity;
        this.centerOrig = centerOrig;
        this.revolving = false;
    }

    setRevolution( velocity, centerOfRevolution ) {
        this.revolving = true;
        this.velocity = velocity;
        this.centerOfRevolution = centerOfRevolution;
    }

    generate( fTree, tag ) {
        fTree.addNode( new FuncNode( "diameter_" + tag,  "float", [ this.diameter ] ) );
        fTree.addNode( new FuncNode( "soften_" + tag,    "float", [ this.soften ] ) );
        fTree.addNode( new FuncNode( "intensity_" + tag, "float", [ this.intensity ] ) );

        if ( this.revolving ) {     // include nodes to calculate center
            fTree.addNode( new FuncNode( "center_orig_" + tag,          "vect2",              this.centerOrig                                           ) );
            fTree.addNode( new FuncNode( "velocity_" + tag,             "float",            [ this.velocity ]                                           ) );
            fTree.addNode( new FuncNode( "center_of_revolution_" + tag, "vect2",              this.centerOfRevolution                                   ) );
            fTree.addNode( new FuncNode( "rev_angle_" + tag,            "fn_f_multiply_3",  [ "time", "max_angle", "velocity_" + tag ]                  ) );
            fTree.addNode( new FuncNode( "center_" + tag,               "fn_v_revolve",     [ "rev_center_" + tag, "center_orig_" + tag, "rev_angle_" + tag ]   ) );
        }
        else {
            fTree.addNode( new FuncNode( "center_" + tag,               "vect2",              this.centerOrig                                           ) );
            
        }
        fTree.addSpacer();
        fTree.addNode( new FuncNode( "relative_" + tag,         "fn_v_subtract",        [ "position", "center_" + tag ]                                      ) );
        fTree.addNode( new FuncNode( "magnitude_" + tag,        "fn_f_magnitude",       [ "relative_" + tag ]                                                ) );
        fTree.addNode( new FuncNode( "complement_" + tag,       "fn_v_complement",      [ "relative_" + tag ]                                                ) );
        fTree.addNode( new FuncNode( "normalize_" + tag,        "fn_v_normalize",       [ "complement_" + tag ]                                              ) );
        fTree.addNode( new FuncNode( "inverse_square_" + tag,   "fn_f_inverse_square",  [ "magnitude_" + tag, "diameter_" + tag, "soften_" + tag ]           ) );
        fTree.addNode( new FuncNode( "swirl_" + tag,            "fn_f_multiply_3",      [ "inverse_square_" + tag, "magnitude_" + tag, "intensity_" + tag ]  ) );
        fTree.addNode( new FuncNode( "result_" + tag,           "fn_v_scale",           [ "normalize_" + tag, "swirl_" + tag ]                               ) );
        //fTree.addNode( new FuncNode( "v_isquare_" + tag,        "fn_v_scale",           [ "normalize_" + tag, "swirl_" + tag ]                               ) );
        //fTree.addNode( new FuncNode( "result_" + tag,           "fn_v_add",             [ "position", "v_isquare_" + tag ]                                    ) );
        fTree.addSpacer();

    }
}

class FuncTree {
    name;
    nNodes;     // number of nodes in function tree
    nodes;      // list of FuncNode objects
    spacers;    // list of node numbers that will be followed by blank lines for readability

    constructor( name ) {
        this.name = name;
        this.nNodes = 0;
        this.nodes = [];
        this.spacers = [];
    }

    addNode( node ) {   
        this.nNodes++;
        this.nodes.push( node );
    }

    addSpacer() {
        this.spacers.push( this.nNodes - 1 );
    }

    // Root class generates an identity warp function that does not change the image
    generate() {  
        this.addNode( new FuncNode( "position", "vect2", [ 0.0, 0.0 ] ) );
        this.addNode( new FuncNode( "result", "fn_v_copy", [ "position" ] ) );
    } 

    jenString() {
        // console.log( "spacers: " + this.spacers );
        let spacerIndex = 0;
        let out = "\n*****\n\nname =\t" + this.name + "\n";
        out += "n = \t" + this.nNodes + "\n\n";
        for( let i = 0; i < this.nNodes; i++ ) {
            out += this.nodes[ i ].jenString();

            // add spacers for formatting
            if( ( this.spacers.length > 0 ) && ( spacerIndex < this.spacers.length ) ) {
                if( i == this.spacers[ spacerIndex ] ) {
                    out += "\n";
                    spacerIndex++;
                }
            }
        }
        out += "\n*****\n";
        return out;
    }
}

class VortexField extends FuncTree {
    nVortices;
    step;                               // float - overall scaling factor
    minBound; maxBound;                           // Minimum and maximum coordinates of field - vect2
    revolving;                          // bool - do vortices revolve?

    minDiameter; maxDiameter;           // float - size of vortex

    minSoften; maxSoften;               // float - singularity avoidance

    minIntensity; maxIntensity;         // float
    intensityRotationDirection;         // object of RotationDirection

    minVelocity; maxVelocity;           // must be integer values for animation to loop
    velocityRotationDirection;          // object of RotationDirection

    minOrbitalRadius; maxOrbitalRadius; // float             

    constructor( name, nVortices ) {
        super( name );
        this.nVortices = nVortices;
        this.revolving = false;

        // Set some plausible values
        this.step = 0.5;
        this.minBound = [ -1.0, -1.0 ];
        this.maxBound = [  1.0,  1.0 ];

        this.minDiameter = 0.5;
        this.maxDiameter = 0.5;

        this.minSoften = 0.25;
        this.maxSoften = 0.25;

        this.minIntensity = 1.0
        this.maxIntensity = 1.0;
        this.intensityRotationDirection = RotationDirection.random;

        this.minVelocity = 1.0;
        this.maxVelocity = 1.0;
        this.velocityRotationDirection =  RotationDirection.random;

        this.minOrbitalRadius = 0.0;
        this.maxOrbitalRadius = 0.5;
    } 

    generate() {                                        // overrides parent class function
        if( this.nVortices == 0 ) { super.generate(); } // if number of vortices is zero, generate identity warp
        else {
            //console.log( this.name + ".generate()" );
            // add essential leaves
            this.addNode( new FuncNode( "position", "vect2", [ 0.0, 0.0 ] ) );
            this.addNode( new FuncNode( "time", "float", [ 0.0 ] ) );
            this.addNode( new FuncNode( "max_angle", "float", [ 360.0 ] ) );
            this.addNode( new FuncNode( "step", "float", [ this.step ] ) );

            this.addSpacer();

            for( let i=0; i < this.nVortices; i++ ) {

                let diameter = randomRange( this.minDiameter, this.maxDiameter ); 
                                
                let soften =   randomRange( this.minSoften, this.maxSoften ); 
  
                // random arrangement of centers of rotation. There are other possibilities
                let centerOfRevolution = boxOfRandom( this.minBound, this.maxBound );
                let centerOrig = centerOfRevolution;

                if( this.revolving ) {
                    let orbitalRadius = randomRange( this.minOrbitalRadius, this.maxOrbitalRadius );
                    let theta = randomAngle();
                    centerOrig[ 0 ] += orbitalRadius * Math.cos( theta ); 
                    centerOrig[ 1 ] += orbitalRadius * Math.sin( theta );
                }

                let velocity = 0.0;
                if( this.revolving ) {
                // random integer velocity
                    velocity = randomRange( this.minVelocity, this.maxVelocity );
                    switch( this.velocityRotationDirection ) {
                        case RotationDirection.Counterclockwise:
                            velocity = Math.abs( velocity );
                            break;
                        case RotationDirection.Clockwise:
                            velocity = -Math.abs( velocity );
                            break;
                        case RotationDirection.Random:
                            if( fairCoin() ) velocity *= -1;    // -1 or 1
                            break;   
                        case RotationDirection.LavaLamp:
                            if ( cor[ 0 ] < 0.0 )   { velocity =   Math.abs( velocity ); }
                            else                    { velocity = - Math.abs( velocity ); }
                    }
                }
                
                let intensity = this.maxIntensity;
                if( this.randomIntensity ) { intensity = randomRange( this.minIntensity, this.maxIntensity ); }
                switch( this.intensityRotationDirection ) {
                    case RotationDirection.Counterclockwise:
                        intensity = Math.abs( intensity );
                        break;
                    case RotationDirection.Clockwise:
                        intensity = -Math.abs( intensity );
                        break;
                    case RotationDirection.Random:
                        if( fairCoin() ) intensity *= -1;    // -1 or 1
                        break;   
                    case RotationDirection.LavaLamp:
                        if ( cor[ 0 ] < 0.0 )   { intensity =   Math.abs( intensity ); }
                        else                    { intensity = - Math.abs( intensity ); }
                }

                let vort = new Vortex( i, diameter, soften, intensity, centerOrig );
                if( this.revolving ) { vort.setRevolution( velocity, centerOfRevolution ); }
                vort.generate( this, i );

                if( i == 0 ) {
                    this.addNode( new FuncNode( "sum_" + i,  "fn_v_copy",    [ "result_" + i ]                            ) );
                }
                else {
                    this.addNode( new FuncNode( "sum_" + i,  "fn_v_add",     [ "result_" + i, "sum_" + ( i - 1 ), ]       ) );
                }
                this.addSpacer();
            }
            this.addNode(         new FuncNode( "scaled",   "fn_v_scale",   [ "sum_" + ( this.nVortices - 1 ), "step" ]   ) );
            this.addNode(         new FuncNode( "result",   "fn_v_add",     [ "scaled", "position" ]                      ) );
        }
    }
}

function identityWarp() {
    let identiTree = new FuncTree( "identity_warp" );
    identiTree.generate();
    console.log( identiTree.jenString() );
}

function turbulence() {
    let flowField = new VortexField( "turbulence", 4 );
    flowField.generate();
    console.log( flowField.jenString() );
}

calculateFunction.addEventListener('click', turbulence );