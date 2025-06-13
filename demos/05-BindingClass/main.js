function main() {
    let person = new Person("QJSKid", 150, 15, 40)
    console.log(person.name) 
    person.name = "John"
    console.log(person.name)
    console.log(person.bmi)
    person.introduce()
    
    console.log(Person.ID)
}

main()
